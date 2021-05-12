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

    DebugLog("Probing");

    OSObject *uid = nullptr;
    IOACPIPlatformDevice *dev = OSDynamicCast(IOACPIPlatformDevice, provider);
    if (dev && dev->evaluateObject("_UID", &uid) == kIOReturnSuccess) {
        OSString *str = OSDynamicCast(OSString, uid);
        if (str && str->isEqualTo("TBFP")) {
            DebugLog("Skip Thunderbolt interface");
            return nullptr;
        }
    }
    OSSafeReleaseNULL(uid);

    return this;
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

    const char *rname = nullptr;
    OSString *guid = OSDynamicCast(OSString, item->getObject("guid"));
    if (guid)
        rname = registerEvent(guid, id);

    OSDictionary *mof = OSDynamicCast(OSDictionary, item->getObject("MOF"));
    if (!mof) {
        AlwaysLog("found %s notify id 0x%x with no description", rname ? rname : "unknown", id);
        return;
    }

    OSString *cname = OSDynamicCast(OSString, mof->getObject("__CLASS"));
    if (!cname) {
        AlwaysLog("found %s notify id 0x%x with no __CLASS", rname ? rname : "unknown", id);
        return;
    }

    AlwaysLog("found %s notify id 0x%x for %s", rname ? rname : "unknown", id, cname->getCStringNoCopy());
    // TODO: Event Enable and Disable WExx; Data Collection Enable and Disable WCxx
}

bool YogaWMI::start(IOService *provider)
{
    if (!YWMI) {
        auto key = OSSymbol::withCString("YogaWMISupported");
        auto dict = IOService::propertyMatching(key, kOSBooleanTrue);
        key->release();
        if (!dict) {
            DebugLog("Failed to create matching dictionary");
            return false;
        }

        auto vpc = IOService::waitForMatchingService(dict, 1000000000);
        dict->release();
        if (vpc) {
            vpc->release();
            DebugLog("YogaWMI variant available, exiting");
            return false;
        }
    }

    if (!super::start(provider))
        return false;

    DebugLog("Starting");

    if (!YWMI) {
        YWMI = new WMI(provider);
        YWMI->initialize();
    }
    YWMI->start();
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
    AlwaysLog("message: Unknown ACPI Notification 0x%02X", argument);
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
