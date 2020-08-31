//
//  YogaSMC.cpp
//  YogaSMC
//
//  Created by Zhen on 7/29/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "YogaSMC.hpp"

OSDefineMetaClassAndStructors(YogaSMC, IOService);

bool ADDPR(debugEnabled) = false;
uint32_t ADDPR(debugPrintDelay) = 0;

bool YogaSMC::init(OSDictionary *dictionary)
{
    if (!super::init(dictionary))
        return false;
    name = "";
    DebugLog("Initializing\n");

    _deliverNotification = OSSymbol::withCString(kDeliverNotifications);
     if (!_deliverNotification)
        return false;

    _notificationServices = OSSet::withCapacity(1);

    extern kmod_info_t kmod_info;
    setProperty("YogaSMC,Build", __DATE__);
    setProperty("YogaSMC,Version", kmod_info.version);

    return true;
}

void YogaSMC::addVSMCKey() {
    // Message-based
    VirtualSMCAPI::addKey(KeyBDVT, vsmcPlugin.data, VirtualSMCAPI::valueWithFlag(true, new BDVT(this), SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE));
    VirtualSMCAPI::addKey(KeyCH0B, vsmcPlugin.data, VirtualSMCAPI::valueWithData(nullptr, 1, SmcKeyTypeHex, new CH0B, SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE));

    // ACPI-based
    if (!conf || !ec)
        return;

    OSDictionary *status = OSDictionary::withCapacity(8);
    OSString *method;

    // WARNING: watch out, key addition is sorted here!
    // Laptops only have 1 key for both channel
    addECKeySp(KeyTM0P, "Memory Proximity");

    // Desktops
    addECKeySp(KeyTM0p(0), "SO-DIMM 1 Proximity");
    addECKeySp(KeyTM0p(1), "SO-DIMM 2 Proximity");
    addECKeySp(KeyTM0p(2), "SO-DIMM 3 Proximity");
    addECKeySp(KeyTM0p(3), "SO-DIMM 4 Proximity");

    addECKeySp(KeyTPCD, "Platform Controller Hub Die");
    addECKeySp(KeyTW0P, "Airport Proximity");
    addECKeySp(KeyTaLC, "Airflow Left");
    addECKeySp(KeyTaRC, "Airflow Right");
    addECKeySp(KeyTh0H(1), "Fin Stack Proximity Right");
    addECKeySp(KeyTh0H(2), "Fin Stack Proximity Left");
    addECKeySp(KeyTs0p(0), "Palm Rest");
    addECKeySp(KeyTs0p(1), "Trackpad Actuator");

    setProperty("SimpleECKey", status);
    setProperty("Status", vsmcPlugin.data.size(), 32);
    status->release();
}

bool YogaSMC::start(IOService *provider) {
    if (!super::start(provider))
        return false;

    if (ec)
        name = ec->getName();
    else
        name = "";

    DebugLog("Starting\n");

    workLoop = IOWorkLoop::workLoop();
    commandGate = IOCommandGate::commandGate(this);
    poller = IOTimerEventSource::timerEventSource(this, [](OSObject *object, IOTimerEventSource *sender) {
        auto smc = OSDynamicCast(YogaSMC, object);
        if (smc) smc->updateEC();
    });

    if (!workLoop || !commandGate || !poller ||
        (workLoop->addEventSource(commandGate) != kIOReturnSuccess) ||
        (workLoop->addEventSource(poller) != kIOReturnSuccess)) {
        AlwaysLog("Failed to add commandGate and poller\n");
        return false;
    }

    OSDictionary * propertyMatch = propertyMatching(_deliverNotification, kOSBooleanTrue);
    if (propertyMatch) {
        IOServiceMatchingNotificationHandler notificationHandler = OSMemberFunctionCast(IOServiceMatchingNotificationHandler, this, &YogaSMC::notificationHandler);

        //
        // Register notifications for availability of any IOService objects wanting to consume our message events
        //
        _publishNotify = addMatchingNotification(gIOFirstPublishNotification,
                                             propertyMatch,
                                             notificationHandler,
                                             this,
                                             0, 10000);

        _terminateNotify = addMatchingNotification(gIOTerminatedNotification,
                                               propertyMatch,
                                               notificationHandler,
                                               this,
                                               0, 10000);

        propertyMatch->release();

    }

    addVSMCKey();
    vsmcNotifier = VirtualSMCAPI::registerHandler(vsmcNotificationHandler, this);

    poller->setTimeoutMS(POLLING_INTERVAL);
    poller->enable();
    registerService();
    return true;
}

void YogaSMC::stop(IOService *provider)
{
    AlwaysLog("Stopping\n");

    _publishNotify->remove();
    _terminateNotify->remove();
    _notificationServices->flushCollection();
    OSSafeReleaseNULL(_notificationServices);
    OSSafeReleaseNULL(_deliverNotification);

    workLoop->removeEventSource(commandGate);
    poller->disable();
    workLoop->removeEventSource(poller);
    OSSafeReleaseNULL(commandGate);
    OSSafeReleaseNULL(poller);
    OSSafeReleaseNULL(workLoop);

    terminate();
    PMstop();

    super::stop(provider);
}

bool YogaSMC::vsmcNotificationHandler(void *sensors, void *refCon, IOService *vsmc, IONotifier *notifier) {
    auto self = OSDynamicCast(YogaSMC, reinterpret_cast<OSMetaClassBase*>(sensors));
    if (sensors && vsmc) {
        DBGLOG("yogasmc", "got vsmc notification");
        auto &plugin = self->vsmcPlugin;
        auto ret = vsmc->callPlatformFunction(VirtualSMCAPI::SubmitPlugin, true, sensors, &plugin, nullptr, nullptr);
        if (ret == kIOReturnSuccess) {
            DBGLOG("yogasmc", "submitted plugin");
            return true;
        } else if (ret != kIOReturnUnsupported) {
            SYSLOG("yogasmc", "plugin submission failure %X", ret);
        } else {
            DBGLOG("yogasmc", "plugin submission to non vsmc");
        }
    } else {
        SYSLOG("yogasmc", "got null vsmc notification");
    }
    return false;
}

void YogaSMC::dispatchMessageGated(int* message, void* data)
{
    OSCollectionIterator* i = OSCollectionIterator::withCollection(_notificationServices);

    if (i) {
        while (IOService* service = OSDynamicCast(IOService, i->getNextObject())) {
            service->message(*message, this, data);
        }
        i->release();
    }
}

void YogaSMC::dispatchMessage(int message, void* data)
{
    if (_notificationServices->getCount() == 0) {
        AlwaysLog("No available notification consumer\n");
        return;
    }
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &YogaSMC::dispatchMessageGated), &message, data);
}

void YogaSMC::notificationHandlerGated(IOService *newService, IONotifier *notifier)
{
    if (notifier == _publishNotify) {
        DebugLog("Notification consumer published: %s\n", newService->getName());
        _notificationServices->setObject(newService);
    }

    if (notifier == _terminateNotify) {
        DebugLog("Notification consumer terminated: %s\n", newService->getName());
        _notificationServices->removeObject(newService);
    }
}

bool YogaSMC::notificationHandler(void *refCon, IOService *newService, IONotifier *notifier)
{
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &YogaSMC::notificationHandlerGated), newService, notifier);
    return true;
}

YogaSMC* YogaSMC::withDevice(IOService *provider, IOACPIPlatformDevice *device) {
    YogaSMC* dev = OSTypeAlloc(YogaSMC);

    OSDictionary* dictionary = OSDictionary::withCapacity(1);

    dev->ec = device;

    if (!dev->init(dictionary) ||
        !dev->attach(provider)) {
        OSSafeReleaseNULL(dev);
    }

    dev->conf = OSDynamicCast(OSDictionary, provider->getProperty("Sensors"));
    dictionary->release();
    return dev;
}

void YogaSMC::updateEC() {
    UInt32 result = 0;
    for (int i = 0; i < sensorCount; i++) {
        if (ec->evaluateInteger(sensorMethod[i], &result) == kIOReturnSuccess && result != 0)
            atomic_store_explicit(&currentSensor[i], result, memory_order_release);
    }
    poller->setTimeoutMS(POLLING_INTERVAL);
}

EXPORT extern "C" kern_return_t ADDPR(kern_start)(kmod_info_t *, void *) {
    // Report success but actually do not start and let I/O Kit unload us.
    // This works better and increases boot speed in some cases.
    PE_parse_boot_argn("liludelay", &ADDPR(debugPrintDelay), sizeof(ADDPR(debugPrintDelay)));
    ADDPR(debugEnabled) = checkKernelArgument("-vsmcdbg");
    return KERN_SUCCESS;
}

EXPORT extern "C" kern_return_t ADDPR(kern_stop)(kmod_info_t *, void *) {
    // It is not safe to unload VirtualSMC plugins!
    return KERN_FAILURE;
}

#ifdef __MAC_10_15

// macOS 10.15 adds Dispatch function to all OSObject instances and basically
// every header is now incompatible with 10.14 and earlier.
// Here we add a stub to permit older macOS versions to link.
// Note, this is done in both kern_util and plugin_start as plugins will not link
// to Lilu weak exports from vtable.

kern_return_t WEAKFUNC PRIVATE OSObject::Dispatch(const IORPC rpc) {
    PANIC("util", "OSObject::Dispatch smcbat stub called");
}

kern_return_t WEAKFUNC PRIVATE OSMetaClassBase::Dispatch(const IORPC rpc) {
    PANIC("util", "OSMetaClassBase::Dispatch smcbat stub called");
}

#endif
