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
    bool res = super::init(dictionary);
    IOLog("%s Initializing\n", getName());

    _deliverNotification = OSSymbol::withCString(kDeliverNotifications);
     if (!_deliverNotification)
        return false;

    _notificationServices = OSSet::withCapacity(1);

    extern kmod_info_t kmod_info;
    setProperty("YogaSMC,Build", __DATE__);
    setProperty("YogaSMC,Version", kmod_info.version);

    return res;
}

IOService *YogaWMI::probe(IOService *provider, SInt32 *score)
{
    name = provider->getName();

    if (strncmp(name, "WTBT", 4) == 0) {
        IOLog("%s::%s exiting on Thunderbolt interface\n", getName(), name);
        return nullptr;
    }

    if (provider->getClient() != this) {
        IOLog("%s::%s already loaded, exiting\n", getName(), name);
        return nullptr;
    }

    IOLog("%s::%s Probing\n", getName(), name);

    IOACPIPlatformDevice *vpc {nullptr};

    if(getVPCName() && !findPNP(getVPCName(), &vpc)) {
        IOLog("%s::%s Failed to find VPC\n", getName(), name);
        return nullptr;
    }

    return super::probe(provider, score);
}

void YogaWMI::checkEvent(const char *cname, UInt32 id) {
    if (id == kIOACPIMessageReserved)
        IOLog("%s::%s found reserved notify id 0x%x for %s\n", getName(), name, id, cname);
    else
        IOLog("%s::%s found unknown notify id 0x%x for %s\n", getName(), name, id, cname);
}

void YogaWMI::getNotifyID(OSString *key) {
    OSDictionary *item = OSDynamicCast(OSDictionary, Event->getObject(key));
    if (!item) {
        IOLog("%s::%s found unparsed notify id %s\n", getName(), name, key->getCStringNoCopy());
        return;
    }
    OSNumber *id = OSDynamicCast(OSNumber, item->getObject(kWMINotifyId));
    if (id != nullptr) {
        IOLog("%s::%s found invalid notify id %s\n", getName(), name, key->getCStringNoCopy());
        return;
    }
    char notify_id_string[3];
    snprintf(notify_id_string, 3, "%2x", id->unsigned8BitValue());
    if (strncmp(key->getCStringNoCopy(), notify_id_string, 2) != 0) {
        IOLog("%s::%s notify id %s mismatch %x\n", getName(), name, key->getCStringNoCopy(), id->unsigned8BitValue());
    }

    OSDictionary *mof = OSDynamicCast(OSDictionary, item->getObject("MOF"));
    if (!mof) {
        IOLog("%s::%s found notify id 0x%x with no description\n", getName(), name, id->unsigned8BitValue());
        return;
    }
    OSString *cname = OSDynamicCast(OSString, mof->getObject("__CLASS"));
    if (!cname) {
        IOLog("%s::%s found notify id 0x%x with no __CLASS\n", getName(), name, id->unsigned8BitValue());
        return;
    }
    checkEvent(cname->getCStringNoCopy(), id->unsigned8BitValue());
    // TODO: Event Enable and Disable WExx; Data Collection Enable and Disable WCxx
}

bool YogaWMI::start(IOService *provider)
{
    bool res = super::start(provider);
    IOLog("%s::%s Starting\n", getName(), name);

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
        IOLog("%s::%s Failed to add commandGate\n", getName(), name);
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
    IOLog("%s::%s Stopping\n", getName(), name);

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
    IOLog("%s::%s message: Unknown ACPI Notification 0x%x\n", getName(), name, argument);
}

IOReturn YogaWMI::message(UInt32 type, IOService *provider, void *argument) {
    switch (type)
    {
        case kIOACPIMessageDeviceNotification:
            if (argument)
                ACPIEvent(*(UInt32 *) argument);
            else
                IOLog("%s::%s message: ACPI provider=%s, unknown argument\n", getName(), name, provider->getName());
            break;

        default:
            if (argument)
                IOLog("%s::%s message: type=%x, provider=%s, argument=0x%04x\n", getName(), name, type, provider->getName(), *((UInt32 *) argument));
            else
                IOLog("%s::%s message: type=%x, provider=%s, unknown argument\n", getName(), name, type, provider->getName());
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
        IOLog("%s::%s No available notification consumer\n", getName(), name);
        return;
    }
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &YogaWMI::dispatchMessageGated), &message, data);
}

void YogaWMI::notificationHandlerGated(IOService *newService, IONotifier *notifier)
{
    if (notifier == _publishNotify) {
        IOLog("%s::%s Notification consumer published: %s\n", getName(), name, newService->getName());
        _notificationServices->setObject(newService);
    }

    if (notifier == _terminateNotify) {
        IOLog("%s::%s Notification consumer terminated: %s\n", getName(), name, newService->getName());
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
        IOLog("%s::%s findPNP failed\n", getName(), name);
        return false;
    }
    auto pnp = OSString::withCString(id);

    while (auto entry = iterator->getNextObject()) {
        if (entry->compareName(pnp)) {
            *dev = OSDynamicCast(IOACPIPlatformDevice, entry);
            if (*dev) {
                IOLog("%s::%s %s available at %s\n", getName(), name, id, (*dev)->getName());
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
    IOLog("%s::%s TouchPad Input %s\n", getName(), name, TouchPadenabled ? "enabled" : "disabled");
    setProperty("TouchPadEnabled", TouchPadenabled);
}

void YogaWMI::toggleKeyboard() {
    dispatchMessage(kSMC_getKeyboardStatus, &Keyboardenabled);
    Keyboardenabled = !Keyboardenabled;
    dispatchMessage(kSMC_setKeyboardStatus, &Keyboardenabled);
    IOLog("%s::%s Keyboard Input %s\n", getName(), name, Keyboardenabled ? "enabled" : "disabled");
    setProperty("KeyboardEnabled", Keyboardenabled);
}

void YogaWMI::setTopCase(bool enable) {
    dispatchMessage(kSMC_setKeyboardStatus, &enable);
    dispatchMessage(kSMC_setDisableTouchpad, &enable);
    IOLog("%s::%s TopCase Input %s\n", getName(), name, enable ? "enabled" : "disabled");
    setProperty("TopCaseEnabled", enable);
}

bool YogaWMI::updateTopCase() {
    dispatchMessage(kSMC_getKeyboardStatus, &Keyboardenabled);
    dispatchMessage(kSMC_getDisableTouchpad, &TouchPadenabled);
    if (Keyboardenabled != TouchPadenabled) {
        IOLog("%s::%s status mismatch: %d, %d\n", getName(), name, Keyboardenabled, TouchPadenabled);
        return false;
    }
    return true;
}

IOReturn YogaWMI::setPowerState(unsigned long powerState, IOService *whatDevice){
    IOLog("%s::%s powerState %ld : %s", getName(), name, powerState, powerState ? "on" : "off");

    if (whatDevice != this)
        return kIOReturnInvalid;

    return kIOPMAckImplied;
}
