//
//  YogaSMC.cpp
//  YogaSMC
//
//  Created by Zhen on 7/29/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "YogaSMC.hpp"

OSDefineMetaClassAndStructors(YogaSMC, YogaBaseService);

bool ADDPR(debugEnabled) = false;
uint32_t ADDPR(debugPrintDelay) = 0;

void YogaSMC::addVSMCKey() {
    // ACPI-based
    if (!conf || !ec)
        return;

    OSDictionary *status = OSDictionary::withCapacity(8);
    OSString *method;

    addECKeySp(KeyTB0T(0), "Battery", atomicSpDeciKelvinKey);
    addECKeySp(KeyTB0T(1), "Battery Sensor 1", atomicSpDeciKelvinKey);
    addECKeySp(KeyTB0T(2), "Battery Sensor 2", atomicSpDeciKelvinKey);

    addECKeySp(KeyTCSA, "CPU System Agent Core", atomicSpKey);
    addECKeySp(KeyTCXC, "CPU Core PECI", atomicSpKey);

    // Laptops only have 1 key for both channel
    addECKeySp(KeyTM0P, "Memory Proximity", atomicSpKey);

    // Desktops
    addECKeySp(KeyTM0p(0), "SO-DIMM 1 Proximity", atomicSpKey);
    addECKeySp(KeyTM0p(1), "SO-DIMM 2 Proximity", atomicSpKey);
    addECKeySp(KeyTM0p(2), "SO-DIMM 3 Proximity", atomicSpKey);
    addECKeySp(KeyTM0p(3), "SO-DIMM 4 Proximity", atomicSpKey);

    addECKeySp(KeyTPCD, "Platform Controller Hub Die", atomicSpKey);
    addECKeySp(KeyTW0P, "Airport Proximity", atomicSpKey);
    addECKeySp(KeyTaLC, "Airflow Left", atomicSpKey);
    addECKeySp(KeyTaRC, "Airflow Right", atomicSpKey);
    addECKeySp(KeyTh0H(1), "Fin Stack Proximity Right", atomicSpKey);
    addECKeySp(KeyTh0H(2), "Fin Stack Proximity Left", atomicSpKey);
    addECKeySp(KeyTs0p(0), "Palm Rest", atomicSpKey);
    addECKeySp(KeyTs0p(1), "Trackpad Actuator", atomicSpKey);

    setProperty("DirectECKey", status);
    status->release();
}

bool YogaSMC::start(IOService *provider) {
    if (!super::start(provider))
        return false;

    DebugLog("Starting");

    validateEC();

    awake = true;

    if (!(poller = initPoller()) ||
        (workLoop->addEventSource(poller) != kIOReturnSuccess)) {
        AlwaysLog("Failed to add poller");
        return false;
    }

    // WARNING: watch out, key addition is sorted here!
    addVSMCKey();
    qsort(const_cast<VirtualSMCKeyValue *>(vsmcPlugin.data.data()), vsmcPlugin.data.size(), sizeof(VirtualSMCKeyValue), VirtualSMCKeyValue::compare);
    setProperty("Status", vsmcPlugin.data.size(), 32);

    vsmcNotifier = VirtualSMCAPI::registerHandler(vsmcNotificationHandler, this);

    poller->setTimeoutMS(POLLING_INTERVAL);
    poller->enable();
    registerService();
    return true;
}

void YogaSMC::stop(IOService *provider)
{
    DebugLog("Stopping");

    poller->disable();
    workLoop->removeEventSource(poller);
    OSSafeReleaseNULL(poller);

    terminate();

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

YogaSMC* YogaSMC::withDevice(IOService *provider, IOACPIPlatformDevice *device) {
    YogaSMC* dev = OSTypeAlloc(YogaSMC);

    dev->conf = OSDictionary::withDictionary(OSDynamicCast(OSDictionary, provider->getProperty("Sensors")));

    OSDictionary *dictionary = OSDictionary::withCapacity(1);
    dictionary->setObject("Sensors", dev->conf);

    dev->ec = device;
    dev->name = device->getName();

    if (!dev->init(dictionary) ||
        !dev->attach(provider)) {
        OSSafeReleaseNULL(dev);
    }

    dictionary->release();
    return dev;
}

void YogaSMC::updateEC() {
    if (!awake)
        return;

    UInt32 result = 0;
    for (int i = 0; i < sensorCount; i++) {
        if (ec->evaluateInteger(sensorMethod[i], &result) == kIOReturnSuccess && result != 0)
            atomic_store_explicit(&currentSensor[i], result, memory_order_release);
    }
    poller->setTimeoutMS(POLLING_INTERVAL);
}

IOReturn YogaSMC::setPowerState(unsigned long powerStateOrdinal, IOService *whatDevice){
    DebugLog("powerState %ld : %s", powerStateOrdinal, powerStateOrdinal ? "on" : "off");
    if (whatDevice != this)
        return kIOReturnInvalid;
    if (powerStateOrdinal == 0) {
        if (awake) {
            poller->disable();
            workLoop->removeEventSource(poller);
            awake = false;
            DebugLog("Going to sleep");
        }
    } else {
        if (!awake) {
            awake = true;
            workLoop->addEventSource(poller);
            poller->setTimeoutMS(POLLING_INTERVAL);
            poller->enable();
            DebugLog("Woke up");
        }
    }
    return kIOPMAckImplied;
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
