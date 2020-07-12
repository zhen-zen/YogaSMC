//  SPDX-License-Identifier: GPL-2.0-only
//
//  YogaSMC.cpp
//  YogaSMC
//
//  Created by Zhen on 2020/5/21.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "YogaSMC.hpp"

OSDefineMetaClassAndStructors(YogaWMI, IOService)

bool YogaWMI::init(OSDictionary *dictionary)
{
    bool res = super::init(dictionary);
    IOLog("%s: Initializing\n", getName());

    _deliverNotification = OSSymbol::withCString(kDeliverNotifications);
     if (_deliverNotification == NULL)
        return false;

    _notificationServices = OSSet::withCapacity(1);

    extern kmod_info_t kmod_info;
    setProperty("YogaSMC,Build", __DATE__);
    setProperty("YogaSMC,Version", kmod_info.version);

    return res;
}

void YogaWMI::free(void)
{
    IOLog("%s: Freeing\n", getName());
    super::free();
}

IOService *YogaWMI::probe(IOService *provider, SInt32 *score)
{
    if (strncmp(provider->getName(), "WTBT", 4) == 0) {
        IOLog("%s: exiting on %s\n", getName(), provider->getName());
        return NULL;
    }
    if (strncmp(provider->getName(), "WMI2", 4) == 0) {
        if (strncmp(getName(), "YogaWMI", 7) == 0) {
            IOLog("%s: exiting on %s\n", getName(), provider->getName());
            return NULL;
        }
    }
    IOLog("%s: Probing %s\n", getName(), provider->getName());

    return super::probe(provider, score);
}

void YogaWMI::getNotifyID(OSString *key) {
    OSDictionary *item = OSDynamicCast(OSDictionary, Event->getObject(key));
    if (item == NULL) {
        IOLog("%s: found unparsed notify id %s\n", getName(), key->getCStringNoCopy());
        return;
    }
    OSNumber *id = OSDynamicCast(OSNumber, item->getObject(kWMINotifyId));
    if (id == NULL) {
        IOLog("%s: found invalid notify id %s\n", getName(), key->getCStringNoCopy());
        return;
    }
    char notify_id_string[3];
    snprintf(notify_id_string, 3, "%2x", id->unsigned8BitValue());
    if (strncmp(key->getCStringNoCopy(), notify_id_string, 2) != 0) {
        IOLog("%s: notify id %s mismatch %x\n", getName(), key->getCStringNoCopy(), id->unsigned8BitValue());
    }

    OSDictionary *mof = OSDynamicCast(OSDictionary, item->getObject("MOF"));
    if (mof == NULL) {
        IOLog("%s: found notify id 0x%x with no description\n", getName(), id->unsigned8BitValue());
        return;
    }
    OSString *name = OSDynamicCast(OSString, mof->getObject("__CLASS"));
    if (name == NULL) {
        IOLog("%s: found notify id 0x%x with no __CLASS\n", getName(), id->unsigned8BitValue());
        return;
    }
    switch (id->unsigned8BitValue()) {
        case kIOACPIMessageReserved:
            IOLog("%s: found reserved notify id 0x%x for %s\n", getName(), id->unsigned8BitValue(), name->getCStringNoCopy());
            break;
            
        case kIOACPIMessageD0:
            IOLog("%s: found YMC notify id 0x%x for %s\n", getName(), id->unsigned8BitValue(), name->getCStringNoCopy());
            break;
            
        default:
            IOLog("%s: found unknown notify id 0x%x for %s\n", getName(), id->unsigned8BitValue(), name->getCStringNoCopy());
            break;
    }
    // TODO: Event Enable and Disable WExx; Data Collection Enable and Disable WCxx
}

bool YogaWMI::start(IOService *provider)
{
    bool res = super::start(provider);
    IOLog("%s: Starting\n", getName());

    YWMI = new WMI(provider);
    YWMI->initialize();

    if (YWMI->hasMethod(YMC_WMI_EVENT, ACPI_WMI_EVENT)) {
        if (YWMI->hasMethod(YMC_WMI_METHOD)) {
            setProperty("Feature", "YMC");
            isYMC = true;
        } else {
            IOLog("%s: YMC method not found\n", getName());
        }
    }

    Event = YWMI->getEvent();
    if (Event != NULL) {
        setProperty("Event", Event);
        OSCollectionIterator* i = OSCollectionIterator::withCollection(Event);

        if (i != NULL) {
            while (OSString* key = OSDynamicCast(OSString, i->getNextObject())) {
                getNotifyID(key);
            }
            i->release();
        }
    }

    findVPC();

    workLoop = IOWorkLoop::workLoop();
    commandGate = IOCommandGate::commandGate(this);
    if (!workLoop || !commandGate || (workLoop->addEventSource(commandGate) != kIOReturnSuccess)) {
        IOLog("%s: Failed to add commandGate\n", getName());
        return false;
    }

    OSDictionary * propertyMatch = propertyMatching(_deliverNotification, kOSBooleanTrue);
    if (propertyMatch != NULL) {
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
    return res;
}

void YogaWMI::stop(IOService *provider)
{
    IOLog("%s: Stopping\n", getName());
    if (YogaMode != kYogaMode_laptop) {
        IOLog("%s: Re-enabling top case\n", getName());
        setTopCase(true);
    }

    if (YWMI) {
        delete YWMI;
    }

    if (vpc && VPCNotifiers) {
        VPCNotifiers->remove();
    }

    _publishNotify->remove();
    _terminateNotify->remove();
    _notificationServices->flushCollection();
    OSSafeReleaseNULL(_notificationServices);
    OSSafeReleaseNULL(_deliverNotification);

    workLoop->removeEventSource(commandGate);
    OSSafeReleaseNULL(commandGate);
    OSSafeReleaseNULL(workLoop);

    super::stop(provider);
}

void YogaWMI::YogaEvent(UInt32 argument) {
    if (argument != kIOACPIMessageD0) {
        IOLog("%s: Unknown argument 0x%x\n", getName(), argument);
        return;
    }

    IOLog("%s: message: argument D0 -> WMIY\n", getName());

    if (!isYMC) {
        IOLog("YogaWMI::message: unknown YMC");
        return;
    }

    OSObject* params[3] = {
        OSNumber::withNumber(0ULL, 32),
        OSNumber::withNumber(0ULL, 32),
        OSNumber::withNumber(0ULL, 32)
    };

    UInt32 value;
    
    if (!YWMI->executeinteger(YMC_WMI_METHOD, &value, params, 3)) {
        setProperty("YogaMode", false);
        IOLog("%s: YogaMode: detection failed\n", getName());
        return;
    }

    IOLog("%s: YogaMode: %d\n", getName(), value);
    bool sync = updateTopCase();
    if (YogaMode == value && sync)
        return;

    switch (value) {
        case kYogaMode_laptop:
            setTopCase(true);
            setProperty("YogaMode", "Laptop");
            break;

        case kYogaMode_tablet:
            if (YogaMode == kYogaMode_laptop || !sync)
                setTopCase(false);
            setProperty("YogaMode", "Tablet");
            break;

        case kYogaMode_stand:
            if (YogaMode == kYogaMode_laptop || !sync)
                setTopCase(false);
            setProperty("YogaMode", "Stand");
            break;

        case kYogaMode_tent:
            if (YogaMode == kYogaMode_laptop || !sync)
                setTopCase(false);
            setProperty("YogaMode", "Tent");
            break;

        default:
            IOLog("%s: Unknown yoga mode: %d\n", getName(), value);
            setProperty("YogaMode", value);
            return;
    }
    YogaMode = value;
}

IOReturn YogaWMI::message(UInt32 type, IOService *provider, void *argument) {
    if (argument) {
        IOLog("%s: message: type=%x, provider=%s, argument=0x%04x\n", getName(), type, provider->getName(), *((UInt32 *) argument));
        YogaEvent(*(UInt32 *) argument);
    } else {
        IOLog("%s: message: type=%x, provider=%s\n", getName(), type, provider->getName());
    }
    return kIOReturnSuccess;
}

void YogaWMI::dispatchMessageGated(int* message, void* data)
{
    OSCollectionIterator* i = OSCollectionIterator::withCollection(_notificationServices);

    if (i != NULL) {
        while (IOService* service = OSDynamicCast(IOService, i->getNextObject())) {
            service->message(*message, this, data);
        }
        i->release();
    }
}

void YogaWMI::dispatchMessage(int message, void* data)
{
    if (_notificationServices->getCount() == 0) {
        IOLog("%s: No available notification consumer\n", getName());
        return;
    }
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &YogaWMI::dispatchMessageGated), &message, data);
}

void YogaWMI::notificationHandlerGated(IOService *newService, IONotifier *notifier)
{
    if (notifier == _publishNotify) {
        IOLog("%s: Notification consumer published: %s\n", getName(), newService->getName());
        _notificationServices->setObject(newService);
    }

    if (notifier == _terminateNotify) {
        IOLog("%s: Notification consumer terminated: %s\n", getName(), newService->getName());
        _notificationServices->removeObject(newService);
    }
}

bool YogaWMI::notificationHandler(void *refCon, IOService *newService, IONotifier *notifier)
{
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &YogaWMI::notificationHandlerGated), newService, notifier);
    return true;
}

bool YogaWMI::findVPC() {
    if(!getPnp())
        return false;

    auto iterator = IORegistryIterator::iterateOver(gIOACPIPlane, kIORegistryIterateRecursively);
    if (!iterator) {
        IOLog("%s: findVPC failed\n", getName());
        return false;
    }

    while (auto entry = iterator->getNextObject()) {
        if (entry->compareName(getPnp())) {
            vpc = OSDynamicCast(IOACPIPlatformDevice, entry);
            if (vpc) {
                IOLog("%s: findVPC available at %s\n", getName(), entry->getName());
                VPCNotifiers = vpc->registerInterest(gIOGeneralInterest, VPCNotification, this);
                if (!VPCNotifiers)
                    IOLog("%s: findVPC unable to register interest for VPC notifications\n", getName());
                break;
            }
        }
    }
    iterator->release();

    setProperty("Feature", "VPC");
    return initVPC();
}

IOReturn YogaWMI::VPCNotification(void *target, void *refCon, UInt32 messageType, IOService *provider, void *messageArgument, vm_size_t argSize) {
    auto self = OSDynamicCast(YogaWMI, reinterpret_cast<OSMetaClassBase*>(target));
    if (self == nullptr) {
        IOLog("YogaWMI: target is not YogaWMI");
        return kIOReturnError;
    }
    if (messageArgument) {
        IOLog("%s: VPC %s received %x, argument=0x%04x", self->getName(), provider->getName(), messageType, *((UInt32 *) messageArgument));
        if (*(UInt32 *)messageArgument ==  kIOACPIMessageReserved) {
            self->updateVPC();
        }
    } else {
        IOLog("%s: VPC %s received %x", self->getName(), provider->getName(), messageType);
    }
    return kIOReturnSuccess;
}

void YogaWMI::toggleTouchpad() {
    dispatchMessage(kSMC_getDisableTouchpad, &TouchPadenabled);
    TouchPadenabled = !TouchPadenabled;
    dispatchMessage(kSMC_setDisableTouchpad, &TouchPadenabled);
    IOLog("%s: TouchPad Input %s\n", getName(), TouchPadenabled ? "enabled" : "disabled");
    setProperty("TouchPadEnabled", TouchPadenabled);
}

void YogaWMI::toggleKeyboard() {
    dispatchMessage(kSMC_getKeyboardStatus, &Keyboardenabled);
    Keyboardenabled = !Keyboardenabled;
    dispatchMessage(kSMC_setKeyboardStatus, &Keyboardenabled);
    IOLog("%s: Keyboard Input %s\n", getName(), Keyboardenabled ? "enabled" : "disabled");
    setProperty("KeyboardEnabled", Keyboardenabled);
}

void YogaWMI::setTopCase(bool enable) {
    dispatchMessage(kSMC_setKeyboardStatus, &enable);
    dispatchMessage(kSMC_setDisableTouchpad, &enable);
    IOLog("%s: TopCase Input %s\n", getName(), enable ? "enabled" : "disabled");
    setProperty("TopCaseEnabled", enable);
}

bool YogaWMI::updateTopCase() {
    dispatchMessage(kSMC_getKeyboardStatus, &Keyboardenabled);
    dispatchMessage(kSMC_getDisableTouchpad, &TouchPadenabled);
    if (Keyboardenabled != TouchPadenabled) {
        IOLog("%s: status mismatch: %d, %d\n", getName(), Keyboardenabled, TouchPadenabled);
        return false;
    }
    return true;
}
