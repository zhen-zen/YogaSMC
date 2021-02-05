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

    return this;
}

void YogaWMI::checkEvent(const char *cname, UInt32 id) {
    if (id == kIOACPIMessageReserved)
        AlwaysLog("found reserved notify id 0x%x for %s", id, cname);
    else
        AlwaysLog("found unknown notify id 0x%x for %s", id, cname);
}

void YogaWMI::getNotifyID(const char *key) {
    OSDictionary *item = OSDynamicCast(OSDictionary, Event->getObject(key));
    if (!item) {
        AlwaysLog("found unparsed notify id %s", key);
        return;
    }
    OSNumber *num = OSDynamicCast(OSNumber, item->getObject(kWMINotifyId));
    if (num == nullptr) {
        AlwaysLog("found invalid notify id %s", key);
        return;
    }
    UInt32 id;
    sscanf(key, "%2x", &id);
    if (id != num->unsigned8BitValue())
        AlwaysLog("notify id %s mismatch %x", key, num->unsigned8BitValue());

    OSDictionary *mof = OSDynamicCast(OSDictionary, item->getObject("MOF"));
    if (!mof) {
        AlwaysLog("found notify id 0x%x with no description", id);
        return;
    }
    OSString *cname = OSDynamicCast(OSString, mof->getObject("__CLASS"));
    if (!cname) {
        AlwaysLog("found notify id 0x%x with no __CLASS", id);
        return;
    }
    checkEvent(cname->getCStringNoCopy(), id);
    // TODO: Event Enable and Disable WExx; Data Collection Enable and Disable WCxx
}

bool YogaWMI::start(IOService *provider)
{
    if (!super::start(provider))
        return false;

    DebugLog("Starting");

    YWMI = new WMI(provider);
    YWMI->initialize();
    YWMI->extractBMF();
    processWMI();

    Event = YWMI->getEvent();
    if (Event) {
        setProperty("Event", Event);
        OSCollectionIterator* i = OSCollectionIterator::withCollection(Event);

        if (i) {
            while (OSString* key = OSDynamicCast(OSString, i->getNextObject()))
                getNotifyID(key->getCStringNoCopy());
            i->release();
        }
    }

    return true;
}

void YogaWMI::stop(IOService *provider)
{
    DebugLog("Stopping");

    if (YWMI)
        delete YWMI;

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
