//  SPDX-License-Identifier: GPL-2.0-only
//
//  IdeaWMI.cpp
//  YogaSMC
//
//  Created by Zhen on 2020/6/23.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "IdeaWMI.hpp"

YogaWMI* YogaWMI::withIdeaWMI(WMI *provider) {
    YogaWMI* dev = nullptr;

    if (provider->hasMethod(GSENSOR_WMI_EVENT, ACPI_WMI_EVENT) && provider->hasMethod(GSENSOR_DATA_WMI_METHOD)) {
        dev = OSTypeAlloc(IdeaWMIYoga);
        dev->isPMsupported = true;
    } else if (provider->hasMethod(GSENSOR_WMI_EVENT_EXT, ACPI_WMI_EVENT) && provider->hasMethod(GSENSOR_DATA_WMI_METHOD_EXT)) {
        dev = OSTypeAlloc(IdeaWMIYoga);
        dev->isPMsupported = true;
    } else if (provider->hasMethod(PAPER_LOOKING_WMI_EVENT, ACPI_WMI_EVENT)) {
        dev = OSTypeAlloc(IdeaWMIPaper);
    } else if (provider->hasMethod(BAT_INFO_WMI_STRING, ACPI_WMI_EXPENSIVE | ACPI_WMI_STRING)) {
        dev = OSTypeAlloc(IdeaWMIBattery);
    } else if (provider->hasMethod(GAME_ZONE_DATA_WMI_METHOD)) {
        dev = OSTypeAlloc(IdeaWMIGameZone);
    } else {
        dev = OSTypeAlloc(YogaWMI);
    }

    OSDictionary *dictionary = OSDictionary::withCapacity(1);

    dev->iname = provider->getName();
    dev->YWMI = provider;

    if (!dev->init(dictionary))
        OSSafeReleaseNULL(dev);

    dictionary->release();
    return dev;
}

OSDefineMetaClassAndStructors(IdeaWMIYoga, YogaWMI);

const char* IdeaWMIYoga::registerEvent(OSString *guid, UInt32 id) {
    if (!guid->isEqualTo(GSENSOR_WMI_EVENT)) {
        if (!guid->isEqualTo(GSENSOR_WMI_EVENT_EXT))
            return nullptr;
        extension = true;
    }
    YogaEvent = id;
    return feature;
}

void IdeaWMIYoga::ACPIEvent(UInt32 argument) {
    if (argument != YogaEvent) {
        super::ACPIEvent(argument);
        return;
    }

    DebugLog("message: ACPI notification 0x%02X", argument);
    updateYogaMode();
}

void IdeaWMIYoga::processWMI() {
    setProperty("Feature", feature);
}

void IdeaWMIYoga::updateYogaMode() {
    IOReturn ret;
    OSNumber *in;
    UInt32 value;

    in = OSNumber::withNumber(0ULL, 32);
    ret = YWMI->evaluateInteger(extension ? GSENSOR_DATA_WMI_METHOD_EXT : GSENSOR_DATA_WMI_METHOD,
                                0, GSENSOR_DATA_WMI_UASGE_MODE, &value, in, true);
    OSSafeReleaseNULL(in);

    if (ret != kIOReturnSuccess) {
        setProperty("YogaMode", false);
        AlwaysLog("YogaMode evaluation failed");
        return;
    }

    DebugLog("YogaMode: %d", value);
    bool sync = updateTopCase();
    if (YogaMode == value && sync)
        return;

    bool clamshellMode = false;

    switch (value) {
        case kYogaMode_laptop:
            setTopCase(true);
            setProperty("YogaMode", "Laptop");
            break;

        case kYogaMode_tablet:
            if (YogaMode == kYogaMode_laptop || !sync)
                setTopCase(clamshellMode);
            setProperty("YogaMode", "Tablet");
            break;

        case kYogaMode_stand:
            if (YogaMode == kYogaMode_laptop || !sync)
                setTopCase(clamshellMode);
            setProperty("YogaMode", "Stand");
            break;

        case kYogaMode_tent:
            if (YogaMode == kYogaMode_laptop || !sync)
                setTopCase(clamshellMode);
            setProperty("YogaMode", "Tent");
            break;

        default:
            AlwaysLog("Unknown yoga mode: %d", value);
            setProperty("YogaMode", value, 32);
            return;
    }
    YogaMode = value;
    dispatchMessage(kSMC_YogaEvent, &YogaMode);
}

void IdeaWMIYoga::stop(IOService *provider) {
    if (YogaMode != kYogaMode_laptop) {
        DebugLog("Re-enabling top case");
        setTopCase(true);
    }
    super::stop(provider);
}

IOReturn IdeaWMIYoga::setPowerState(unsigned long powerStateOrdinal, IOService * whatDevice) {
    if (super::setPowerState(powerStateOrdinal, whatDevice) != kIOPMAckImplied)
        return kIOReturnInvalid;

    if (powerStateOrdinal == 0) {
        if (YogaMode != kYogaMode_laptop) {
            DebugLog("Re-enabling top case");
            setTopCase(true);
        }
    } else {
        updateYogaMode();
    }

    return kIOPMAckImplied;
}

OSDefineMetaClassAndStructors(IdeaWMIPaper, YogaWMI);

const char* IdeaWMIPaper::registerEvent(OSString *guid, UInt32 id) {
    if (guid->isEqualTo(PAPER_LOOKING_WMI_EVENT)) {
        paperEvent = id;
        return feature;
    }
    return nullptr;
}

void IdeaWMIPaper::ACPIEvent(UInt32 argument) {
    if (argument != paperEvent) {
        super::ACPIEvent(argument);
        return;
    }

    DebugLog("message: ACPI notification 0x%02X", argument);
    // force enable keyboard and touchpad
    setTopCase(true);
    dispatchMessage(kSMC_FnlockEvent, NULL);
}

void IdeaWMIPaper::processWMI() {
    setProperty("Feature", feature);
}

OSDefineMetaClassAndStructors(IdeaWMIBattery, YogaWMI);

void IdeaWMIBattery::processWMI() {
    setProperty("Feature", "Battery Information");
    // only execute once for WMI_EXPENSIVE
    OSArray *BatteryInfo = OSArray::withCapacity(3);
    getBatteryInfo(WBAT_BAT0_BatMaker, BatteryInfo);
    getBatteryInfo(WBAT_BAT0_HwId, BatteryInfo);
    getBatteryInfo(WBAT_BAT0_MfgDate, BatteryInfo);
    setProperty("BatteryInfo", BatteryInfo);
    BatteryInfo->release();
}

bool IdeaWMIBattery::getBatteryInfo(UInt32 index, OSArray *bat) {
    IOReturn ret;
    OSObject *result;
    OSString *info;

    ret = YWMI->queryBlock(BAT_INFO_WMI_STRING, index, &result);
    if (ret != kIOReturnSuccess) {
        AlwaysLog("WBAT evaluation failed");
    } else if ((info = OSDynamicCast(OSString, result)) == nullptr) {
        ret = kIOReturnInvalid;
        AlwaysLog("WBAT result not a string");
    } else {
        AlwaysLog("WBAT %d %s", index, info->getCStringNoCopy());
        bat->setObject(info);
    }

    OSSafeReleaseNULL(result);

    return (ret == kIOReturnSuccess);
}

OSDefineMetaClassAndStructors(IdeaWMIGameZone, YogaWMI);

bool IdeaWMIGameZone::getGamzeZoneData(UInt32 query, UInt32 *result) {
    IOReturn ret;
    OSNumber *in;

    in = OSNumber::withNumber(0ULL, 32);
    ret = YWMI->evaluateInteger(GAME_ZONE_DATA_WMI_METHOD, 0, query, result, in, true);
    OSSafeReleaseNULL(in);

    if (ret != kIOReturnSuccess)
        AlwaysLog("Query %d evaluation failed", query);

    return (ret == kIOReturnSuccess);
}

void IdeaWMIGameZone::processWMI() {
    setProperty("Feature", feature);

    UInt32 result;

    if (getGamzeZoneData(GAME_ZONE_WMI_GET_VERSION, &result))
        setProperty("Game Zone Version", result, 32);
    else
        AlwaysLog("Failed to get version");

    if (getGamzeZoneData(GAME_ZONE_WMI_GET_FAN_NUM, &result))
        setProperty("Fan number", result, 32);
    else
        AlwaysLog("Failed to get fan number");

    if (getGamzeZoneData(GAME_ZONE_WMI_CAP_KEYBOARD, &result))
        setProperty("Keyboard Feature", result, 32);
    else
        AlwaysLog("Failed to get keyboard feature");
}

void IdeaWMIGameZone::ACPIEvent(UInt32 argument) {
    OSObject *result;
    OSNumber *id;
    if (YWMI->getEventData(argument, &result) && (id = (OSDynamicCast(OSNumber, result))))
        DebugLog("message: ACPI notification 0x%04x - 0x%04x", argument, id->unsigned32BitValue());
    else
        AlwaysLog("message: Unknown ACPI notification 0x%04x", argument);
    OSSafeReleaseNULL(result);
}

const char* IdeaWMIGameZone::registerEvent(OSString *guid, UInt32 id) {
    if (guid->isEqualTo(GAME_ZONE_TEMP_WMI_EVENT))
        tempEvent = id;
    else if (guid->isEqualTo(GAME_ZONE_OC_WMI_EVENT))
        OCEvent = id;
    else if (guid->isEqualTo(GAME_ZONE_GPU_WMI_EVENT))
        GPUEvent = id;
    else if (guid->isEqualTo(GAME_ZONE_FAN_WMI_EVENT))
        fanEvent = id;
    else if (guid->isEqualTo(GAME_ZONE_KEY_WMI_EVENT))
        keyEvent = id;
    else
        return nullptr;
    return feature;
}
