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
    IOLog("%s::%s Initializing\n", getName(), name);

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
    IOLog("%s::%s Freeing\n", getName(), name);
    super::free();
}

IOService *YogaWMI::probe(IOService *provider, SInt32 *score)
{
    name = provider->getName();

    if (strncmp(name, "WTBT", 4) == 0) {
        IOLog("%s::%s exiting on %s\n", getName(), name, name);
        return NULL;
    }
    if (strncmp(name, "WMI2", 4) == 0) {
        if (strncmp(getName(), "YogaWMI", 7) == 0) {
            IOLog("%s::%s exiting on %s\n", getName(), name, name);
            return NULL;
        }
    }
    IOLog("%s::%s Probing %s\n", getName(), name, name);

    return super::probe(provider, score);
}

void YogaWMI::getNotifyID(OSString *key) {
    OSDictionary *item = OSDynamicCast(OSDictionary, Event->getObject(key));
    if (item == NULL) {
        IOLog("%s::%s found unparsed notify id %s\n", getName(), name, key->getCStringNoCopy());
        return;
    }
    OSNumber *id = OSDynamicCast(OSNumber, item->getObject(kWMINotifyId));
    if (id == NULL) {
        IOLog("%s::%s found invalid notify id %s\n", getName(), name, key->getCStringNoCopy());
        return;
    }
    char notify_id_string[3];
    snprintf(notify_id_string, 3, "%2x", id->unsigned8BitValue());
    if (strncmp(key->getCStringNoCopy(), notify_id_string, 2) != 0) {
        IOLog("%s::%s notify id %s mismatch %x\n", getName(), name, key->getCStringNoCopy(), id->unsigned8BitValue());
    }

    OSDictionary *mof = OSDynamicCast(OSDictionary, item->getObject("MOF"));
    if (mof == NULL) {
        IOLog("%s::%s found notify id 0x%x with no description\n", getName(), name, id->unsigned8BitValue());
        return;
    }
    OSString *cname = OSDynamicCast(OSString, mof->getObject("__CLASS"));
    if (cname == NULL) {
        IOLog("%s::%s found notify id 0x%x with no __CLASS\n", getName(), name, id->unsigned8BitValue());
        return;
    }
    switch (id->unsigned8BitValue()) {
        case kIOACPIMessageReserved:
            IOLog("%s::%s found reserved notify id 0x%x for %s\n", getName(), name, id->unsigned8BitValue(), cname->getCStringNoCopy());
            break;
            
        case kIOACPIMessageD0:
            IOLog("%s::%s found YMC notify id 0x%x for %s\n", getName(), name, id->unsigned8BitValue(), cname->getCStringNoCopy());
            break;
            
        default:
            IOLog("%s::%s found unknown notify id 0x%x for %s\n", getName(), name, id->unsigned8BitValue(), cname->getCStringNoCopy());
            break;
    }
    // TODO: Event Enable and Disable WExx; Data Collection Enable and Disable WCxx
}

bool YogaWMI::start(IOService *provider)
{
    bool res = super::start(provider);
    IOLog("%s::%s Starting\n", getName(), name);

    YWMI = new WMI(provider);
    YWMI->initialize();

    if (YWMI->hasMethod(YMC_WMI_EVENT, ACPI_WMI_EVENT)) {
        if (YWMI->hasMethod(YMC_WMI_METHOD)) {
            setProperty("Feature", "YMC");
            isYMC = true;
        } else {
            IOLog("%s::%s YMC method not found\n", getName(), name);
        }
    }

    if (YWMI->hasMethod(WBAT_WMI_STRING, ACPI_WMI_EXPENSIVE | ACPI_WMI_STRING)) {
        setProperty("Feature", "WBAT");
        OSArray *BatteryInfo = OSArray::withCapacity(3);
        // only execute once for WMI_EXPENSIVE
        BatteryInfo->setObject(getBatteryInfo(WBAT_BAT0_BatMaker));
        BatteryInfo->setObject(getBatteryInfo(WBAT_BAT0_HwId));
        BatteryInfo->setObject(getBatteryInfo(WBAT_BAT0_MfgDate));
        setProperty("BatteryInfo", BatteryInfo);
        OSSafeReleaseNULL(BatteryInfo);
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
        IOLog("%s::%s Failed to add commandGate\n", getName(), name);
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

    PMinit();
    provider->joinPMtree(this);
    registerPowerDriver(this, IOPMPowerStates, kIOPMNumberPowerStates);

    return res;
}

void YogaWMI::stop(IOService *provider)
{
    IOLog("%s::%s Stopping\n", getName(), name);
    if (YogaMode != kYogaMode_laptop) {
        IOLog("%s::%s Re-enabling top case\n", getName(), name);
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

    PMstop();

    super::stop(provider);
}

void YogaWMI::YogaEvent(UInt32 argument) {
    if (argument != kIOACPIMessageD0) {
        IOLog("%s::%s Unknown argument 0x%x\n", getName(), name, argument);
        return;
    }

    IOLog("%s::%s message: argument D0 -> WMIY\n", getName(), name);

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
        IOLog("%s::%s YogaMode: detection failed\n", getName(), name);
        return;
    }

    IOLog("%s::%s YogaMode: %d\n", getName(), name, value);
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
            IOLog("%s::%s Unknown yoga mode: %d\n", getName(), name, value);
            setProperty("YogaMode", value);
            return;
    }
    YogaMode = value;
}

IOReturn YogaWMI::message(UInt32 type, IOService *provider, void *argument) {
    if (argument) {
        IOLog("%s::%s message: type=%x, provider=%s, argument=0x%04x\n", getName(), name, type, provider->getName(), *((UInt32 *) argument));
        YogaEvent(*(UInt32 *) argument);
    } else {
        IOLog("%s::%s message: type=%x, provider=%s\n", getName(), name, type, provider->getName());
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

bool YogaWMI::findVPC() {
    if(!getPnp())
        return false;

    auto iterator = IORegistryIterator::iterateOver(gIOACPIPlane, kIORegistryIterateRecursively);
    if (!iterator) {
        IOLog("%s::%s findVPC failed\n", getName(), name);
        return false;
    }

    while (auto entry = iterator->getNextObject()) {
        if (entry->compareName(getPnp())) {
            vpc = OSDynamicCast(IOACPIPlatformDevice, entry);
            if (vpc) {
                IOLog("%s::%s findVPC available at %s\n", getName(), name, entry->getName());
                VPCNotifiers = vpc->registerInterest(gIOGeneralInterest, VPCNotification, this);
                if (!VPCNotifiers)
                    IOLog("%s::%s findVPC unable to register interest for VPC notifications\n", getName(), name);
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
        IOLog("%s::%s VPC %s received %x, argument=0x%04x", self->getName(), self->name, provider->getName(), messageType, *((UInt32 *) messageArgument));
        if (*(UInt32 *)messageArgument ==  kIOACPIMessageReserved) {
            self->updateVPC();
        }
    } else {
        IOLog("%s::%s VPC %s received %x", self->getName(), self->name, provider->getName(), messageType);
    }
    return kIOReturnSuccess;
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

OSString * YogaWMI::getBatteryInfo(UInt32 index) {
    OSObject* result;

    OSObject* params[1] = {
        OSNumber::withNumber(index, 32),
    };

    if (!YWMI->executeMethod(WBAT_WMI_STRING, &result, params, 1)) {
        IOLog("%s::%s WBAT evaluation failed\n", getName(), name);
        return OSString::withCString("evaluation failed");
    }

    OSString *info = OSDynamicCast(OSString, result);

    if (!info) {
        IOLog("%s::%s WBAT result not a string\n", getName(), name);
        return OSString::withCString("result not a string");
    }
    IOLog("%s::%s WBAT %s", getName(), name, info->getCStringNoCopy());
    return info;
}

IOReturn YogaWMI::setPowerState(unsigned long powerState, IOService *whatDevice){
    IOLog("%s::%s powerState %ld : %s", getName(), name, powerState, powerState ? "on" : "off");

    if (whatDevice != this)
        return kIOReturnInvalid;

    if (isYMC) {
        if (powerState == 0) {
            if (YogaMode != kYogaMode_laptop) {
                IOLog("%s::%s Re-enabling top case\n", getName(), name);
                setTopCase(true);
            }
        } else {
            YogaEvent(kIOACPIMessageD0);
        }
    }
    return kIOPMAckImplied;
}
