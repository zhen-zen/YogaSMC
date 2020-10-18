//  SPDX-License-Identifier: GPL-2.0-only
//
//  YogaWMI.cpp
//  YogaSMC
//
//  Created by Zhen on 2020/5/21.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "YogaWMI.hpp"

OSDefineMetaClassAndStructors(YogaWMI, YogaBaseService)

IOService *YogaWMI::probe(IOService *provider, SInt32 *score)
{
    if (!super::probe(provider, score))
        return nullptr;

    if (strncmp(name, "WTBT", 4) == 0) {
        DebugLog("Exit on Thunderbolt interface");
        return nullptr;
    }

    if (provider->getClient() != this) {
        DebugLog("Already loaded, exiting");
        return nullptr;
    }

    DebugLog("Probing");

    IOACPIPlatformDevice *vpc {nullptr};

    if(getVPCName() && !findPNP(getVPCName(), &vpc)) {
        AlwaysLog("Failed to find VPC");
        return nullptr;
    }

    return this;
}

void YogaWMI::checkEvent(const char *cname, UInt32 id) {
    if (id == kIOACPIMessageReserved)
        AlwaysLog("found reserved notify id 0x%x for %s", id, cname);
    else
        AlwaysLog("found unknown notify id 0x%x for %s", id, cname);
}

void YogaWMI::getNotifyID(OSString *key) {
    OSDictionary *item = OSDynamicCast(OSDictionary, Event->getObject(key));
    if (!item) {
        AlwaysLog("found unparsed notify id %s", key->getCStringNoCopy());
        return;
    }
    OSNumber *id = OSDynamicCast(OSNumber, item->getObject(kWMINotifyId));
    if (id != nullptr) {
        AlwaysLog("found invalid notify id %s", key->getCStringNoCopy());
        return;
    }
    char notify_id_string[3];
    snprintf(notify_id_string, 3, "%2x", id->unsigned8BitValue());
    if (strncmp(key->getCStringNoCopy(), notify_id_string, 2) != 0) {
        AlwaysLog("notify id %s mismatch %x", key->getCStringNoCopy(), id->unsigned8BitValue());
    }

    OSDictionary *mof = OSDynamicCast(OSDictionary, item->getObject("MOF"));
    if (!mof) {
        AlwaysLog("found notify id 0x%x with no description", id->unsigned8BitValue());
        return;
    }
    OSString *cname = OSDynamicCast(OSString, mof->getObject("__CLASS"));
    if (!cname) {
        AlwaysLog("found notify id 0x%x with no __CLASS", id->unsigned8BitValue());
        return;
    }
    checkEvent(cname->getCStringNoCopy(), id->unsigned8BitValue());
    // TODO: Event Enable and Disable WExx; Data Collection Enable and Disable WCxx
}

bool YogaWMI::start(IOService *provider)
{
    if (!super::start(provider))
        return false;

    AlwaysLog("Starting");

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

    PMinit();
    provider->joinPMtree(this);
    registerPowerDriver(this, IOPMPowerStates, kIOPMNumberPowerStates);

    return true;
}

void YogaWMI::stop(IOService *provider)
{
    AlwaysLog("Stopping");

    if (YWMI)
        delete YWMI;

    PMstop();
    super::stop(provider);
}

void YogaWMI::ACPIEvent(UInt32 argument) {
    AlwaysLog("message: Unknown ACPI Notification 0x%x", argument);
}

IOReturn YogaWMI::message(UInt32 type, IOService *provider, void *argument) {
    switch (type)
    {
        case kIOACPIMessageDeviceNotification:
            if (argument)
                ACPIEvent(*(UInt32 *) argument);
            else
                AlwaysLog("message: ACPI provider=%s, unknown argument", provider->getName());
            break;

        default:
            if (argument)
                AlwaysLog("message: type=%x, provider=%s, argument=0x%04x", type, provider->getName(), *((UInt32 *) argument));
            else
                AlwaysLog("message: type=%x, provider=%s, unknown argument", type, provider->getName());
    }
    return kIOReturnSuccess;
}

void YogaWMI::toggleTouchpad() {
    dispatchMessage(kSMC_getDisableTouchpad, &TouchPadenabled);
    TouchPadenabled = !TouchPadenabled;
    dispatchMessage(kSMC_setDisableTouchpad, &TouchPadenabled);
    DebugLog("TouchPad Input %s", TouchPadenabled ? "enabled" : "disabled");
    setProperty("TouchPadEnabled", TouchPadenabled);
}

void YogaWMI::toggleKeyboard() {
    dispatchMessage(kSMC_getKeyboardStatus, &Keyboardenabled);
    Keyboardenabled = !Keyboardenabled;
    dispatchMessage(kSMC_setKeyboardStatus, &Keyboardenabled);
    DebugLog("Keyboard Input %s", Keyboardenabled ? "enabled" : "disabled");
    setProperty("KeyboardEnabled", Keyboardenabled);
}

void YogaWMI::setTopCase(bool enable) {
    dispatchMessage(kSMC_setKeyboardStatus, &enable);
    dispatchMessage(kSMC_setDisableTouchpad, &enable);
    DebugLog("TopCase Input %s", enable ? "enabled" : "disabled");
    setProperty("TopCaseEnabled", enable);
}

bool YogaWMI::updateTopCase() {
    dispatchMessage(kSMC_getKeyboardStatus, &Keyboardenabled);
    dispatchMessage(kSMC_getDisableTouchpad, &TouchPadenabled);
    if (Keyboardenabled != TouchPadenabled) {
        AlwaysLog("status mismatch: %d, %d", Keyboardenabled, TouchPadenabled);
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
