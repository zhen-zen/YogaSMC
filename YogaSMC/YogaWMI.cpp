//  SPDX-License-Identifier: GPL-2.0-only
//
//  YogaWMI.cpp
//  YogaSMC
//
//  Created by Zhen on 2020/5/21.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "YogaWMI.hpp"

OSDefineMetaClassAndStructors(YogaWMI, IOService)

bool YogaWMI::init(OSDictionary *dictionary)
{
    if(!super::init(dictionary))
        return false;;
    IOLog("%s Initializing\n", getName());

    _deliverNotification = OSSymbol::withCString(kDeliverNotifications);
     if (!_deliverNotification)
        return false;

    _notificationServices = OSSet::withCapacity(1);

    extern kmod_info_t kmod_info;
    setProperty("YogaSMC,Build", __DATE__);
    setProperty("YogaSMC,Version", kmod_info.version);

    return true;
}

IOService *YogaWMI::probe(IOService *provider, SInt32 *score)
{
    name = provider->getName();

    if (strncmp(name, "WTBT", 4) == 0) {
        DebugLog("Exit on Thunderbolt interface\n");
        return nullptr;
    }

    if (provider->getClient() != this) {
        DebugLog("Already loaded, exiting\n");
        return nullptr;
    }

    DebugLog("Probing\n");

    IOACPIPlatformDevice *vpc {nullptr};

    if(getVPCName() && !findPNP(getVPCName(), &vpc)) {
        AlwaysLog("Failed to find VPC\n");
        return nullptr;
    }

    return super::probe(provider, score);
}

void YogaWMI::checkEvent(const char *cname, UInt32 id) {
    if (id == kIOACPIMessageReserved)
        AlwaysLog("found reserved notify id 0x%x for %s\n", id, cname);
    else
        AlwaysLog("found unknown notify id 0x%x for %s\n", id, cname);
}

void YogaWMI::getNotifyID(OSString *key) {
    OSDictionary *item = OSDynamicCast(OSDictionary, Event->getObject(key));
    if (!item) {
        AlwaysLog("found unparsed notify id %s\n", key->getCStringNoCopy());
        return;
    }
    OSNumber *id = OSDynamicCast(OSNumber, item->getObject(kWMINotifyId));
    if (id != nullptr) {
        AlwaysLog("found invalid notify id %s\n", key->getCStringNoCopy());
        return;
    }
    char notify_id_string[3];
    snprintf(notify_id_string, 3, "%2x", id->unsigned8BitValue());
    if (strncmp(key->getCStringNoCopy(), notify_id_string, 2) != 0) {
        AlwaysLog("notify id %s mismatch %x\n", key->getCStringNoCopy(), id->unsigned8BitValue());
    }

    OSDictionary *mof = OSDynamicCast(OSDictionary, item->getObject("MOF"));
    if (!mof) {
        AlwaysLog("found notify id 0x%x with no description\n", id->unsigned8BitValue());
        return;
    }
    OSString *cname = OSDynamicCast(OSString, mof->getObject("__CLASS"));
    if (!cname) {
        AlwaysLog("found notify id 0x%x with no __CLASS\n", id->unsigned8BitValue());
        return;
    }
    checkEvent(cname->getCStringNoCopy(), id->unsigned8BitValue());
    // TODO: Event Enable and Disable WExx; Data Collection Enable and Disable WCxx
}

bool YogaWMI::start(IOService *provider)
{
    bool res = super::start(provider);
    AlwaysLog("Starting\n");

    YWMI = new WMI(provider);
    YWMI->initialize();

    processWMI();

    Event = YWMI->getEvent();
    if (Event) {
        setProperty("Event", Event);
        OSCollectionIterator* i = OSCollectionIterator::withCollection(Event);

        if (i) {
            while (OSString* key = OSDynamicCast(OSString, i->getNextObject())) {
                getNotifyID(key);
            }
            i->release();
        }
    }

    workLoop = IOWorkLoop::workLoop();
    commandGate = IOCommandGate::commandGate(this);
    if (!workLoop || !commandGate || (workLoop->addEventSource(commandGate) != kIOReturnSuccess)) {
        AlwaysLog("Failed to add commandGate\n");
        return false;
    }

    OSDictionary * propertyMatch = propertyMatching(_deliverNotification, kOSBooleanTrue);
    if (propertyMatch) {
      IOServiceMatchingNotificationHandler notificationHandler = OSMemberFunctionCast(IOServiceMatchingNotificationHandler, this, &YogaWMI::notificationHandler);

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

    PMinit();
    provider->joinPMtree(this);
    registerPowerDriver(this, IOPMPowerStates, kIOPMNumberPowerStates);

    return res;
}

void YogaWMI::stop(IOService *provider)
{
    AlwaysLog("Stopping\n");

    if (YWMI) {
        delete YWMI;
    }

    _publishNotify->remove();
    _terminateNotify->remove();
    _notificationServices->flushCollection();
    OSSafeReleaseNULL(_notificationServices);
    OSSafeReleaseNULL(_deliverNotification);

    workLoop->removeEventSource(commandGate);
    OSSafeReleaseNULL(commandGate);
    OSSafeReleaseNULL(workLoop);

    PMstop();

    super::stop(provider);
}

void YogaWMI::ACPIEvent(UInt32 argument) {
    AlwaysLog("message: Unknown ACPI Notification 0x%x\n", argument);
}

IOReturn YogaWMI::message(UInt32 type, IOService *provider, void *argument) {
    switch (type)
    {
        case kIOACPIMessageDeviceNotification:
            if (argument)
                ACPIEvent(*(UInt32 *) argument);
            else
                AlwaysLog("message: ACPI provider=%s, unknown argument\n", provider->getName());
            break;

        default:
            if (argument)
                AlwaysLog("message: type=%x, provider=%s, argument=0x%04x\n", type, provider->getName(), *((UInt32 *) argument));
            else
                AlwaysLog("message: type=%x, provider=%s, unknown argument\n", type, provider->getName());
    }
    return kIOReturnSuccess;
}

void YogaWMI::dispatchMessageGated(int* message, void* data)
{
    OSCollectionIterator* i = OSCollectionIterator::withCollection(_notificationServices);

    if (i) {
        while (IOService* service = OSDynamicCast(IOService, i->getNextObject())) {
            service->message(*message, this, data);
        }
        i->release();
    }
}

void YogaWMI::dispatchMessage(int message, void* data)
{
    if (_notificationServices->getCount() == 0) {
        AlwaysLog("No available notification consumer\n");
        return;
    }
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &YogaWMI::dispatchMessageGated), &message, data);
}

void YogaWMI::notificationHandlerGated(IOService *newService, IONotifier *notifier)
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

bool YogaWMI::notificationHandler(void *refCon, IOService *newService, IONotifier *notifier)
{
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &YogaWMI::notificationHandlerGated), newService, notifier);
    return true;
}

bool YogaWMI::findPNP(const char *id, IOACPIPlatformDevice **dev) {
    auto iterator = IORegistryIterator::iterateOver(gIOACPIPlane, kIORegistryIterateRecursively);
    if (!iterator) {
        AlwaysLog("findPNP failed\n");
        return false;
    }
    auto pnp = OSString::withCString(id);

    while (auto entry = iterator->getNextObject()) {
        if (entry->compareName(pnp)) {
            *dev = OSDynamicCast(IOACPIPlatformDevice, entry);
            if (*dev) {
                DebugLog("%s available at %s\n", id, (*dev)->getName());
                break;
            }
        }
    }
    iterator->release();
    pnp->release();

    return !!(*dev);
}

void YogaWMI::toggleTouchpad() {
    dispatchMessage(kSMC_getDisableTouchpad, &TouchPadenabled);
    TouchPadenabled = !TouchPadenabled;
    dispatchMessage(kSMC_setDisableTouchpad, &TouchPadenabled);
    DebugLog("TouchPad Input %s\n", TouchPadenabled ? "enabled" : "disabled");
    setProperty("TouchPadEnabled", TouchPadenabled);
}

void YogaWMI::toggleKeyboard() {
    dispatchMessage(kSMC_getKeyboardStatus, &Keyboardenabled);
    Keyboardenabled = !Keyboardenabled;
    dispatchMessage(kSMC_setKeyboardStatus, &Keyboardenabled);
    DebugLog("Keyboard Input %s\n", Keyboardenabled ? "enabled" : "disabled");
    setProperty("KeyboardEnabled", Keyboardenabled);
}

void YogaWMI::setTopCase(bool enable) {
    dispatchMessage(kSMC_setKeyboardStatus, &enable);
    dispatchMessage(kSMC_setDisableTouchpad, &enable);
    DebugLog("TopCase Input %s\n", enable ? "enabled" : "disabled");
    setProperty("TopCaseEnabled", enable);
}

bool YogaWMI::updateTopCase() {
    dispatchMessage(kSMC_getKeyboardStatus, &Keyboardenabled);
    dispatchMessage(kSMC_getDisableTouchpad, &TouchPadenabled);
    if (Keyboardenabled != TouchPadenabled) {
        AlwaysLog("status mismatch: %d, %d\n", Keyboardenabled, TouchPadenabled);
        return false;
    }
    return true;
}

IOReturn YogaWMI::setPowerState(unsigned long powerState, IOService *whatDevice){
    DebugLog("powerState %ld : %s", powerState, powerState ? "on" : "off");
    if (whatDevice != this)
        return kIOReturnInvalid;

    return kIOPMAckImplied;
}
