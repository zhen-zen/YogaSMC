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

    if ((provider->hasMethod(GSENSOR_WMI_EVENT, ACPI_WMI_EVENT) && provider->hasMethod(GSENSOR_DATA_WMI_METHOD)) ||
        (provider->hasMethod(GSENSOR_WMI_EVENT_EXT, ACPI_WMI_EVENT) && provider->hasMethod(GSENSOR_DATA_WMI_METHOD_EXT))) {
        dev = OSTypeAlloc(IdeaWMIYoga);
        dev->isPMsupported = true;
    } else if (provider->hasMethod(PAPER_LOOKING_WMI_EVENT, ACPI_WMI_EVENT)) {
        dev = OSTypeAlloc(IdeaWMIPaper);
    } else if (provider->hasMethod(SUPER_RES_WMI_EVENT, ACPI_WMI_EVENT)) {
        dev = OSTypeAlloc(IdeaWMISuperRes);
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
        AlwaysLog(updateFailure, "YogaMode");
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

    DebugLog("message: ACPI notification 0x%02X, toggle FnLock", argument);
    // force enable keyboard and touchpad
    setTopCase(true);
    dispatchMessage(kSMC_FnlockEvent, NULL);
}

void IdeaWMIPaper::processWMI() {
    setProperty("Feature", feature);
}

OSDefineMetaClassAndStructors(IdeaWMISuperRes, YogaWMI);

const char* IdeaWMISuperRes::registerEvent(OSString *guid, UInt32 id) {
    if (guid->isEqualTo(SUPER_RES_WMI_EVENT)) {
        SREvent = id;
        return feature;
    }
    return nullptr;
}

void IdeaWMISuperRes::ACPIEvent(UInt32 argument) {
    if (argument != SREvent) {
        super::ACPIEvent(argument);
        return;
    }

    IOReturn ret;
    OSNumber *in;
    UInt32 value;

    in = OSNumber::withNumber(0ULL, 32);
    ret = YWMI->evaluateInteger(SUPER_RES_DATA_WMI_METHOD, 0, SUPER_RES_DATA_WMI_VALUE, &value, in, true);
    OSSafeReleaseNULL(in);

    if (ret != kIOReturnSuccess) {
        AlwaysLog(updateFailure, "Super resolution");
        DebugLog("message: ACPI notification 0x%02X, toggle FnLock", argument);
    } else {
        DebugLog("message: ACPI notification 0x%02X %d, toggle FnLock", argument, value);
    }

    // force enable keyboard and touchpad
    setTopCase(true);
    dispatchMessage(kSMC_FnlockEvent, NULL);
}

void IdeaWMISuperRes::processWMI() {
    setProperty("Feature", feature);

    IOReturn ret;
    OSNumber *in;
    UInt32 value;

    in = OSNumber::withNumber(0ULL, 32);
    ret = YWMI->evaluateInteger(SUPER_RES_DATA_WMI_METHOD, 0, SUPER_RES_DATA_WMI_CAP, &value, in, true);
    OSSafeReleaseNULL(in);

    if (ret != kIOReturnSuccess) {
        AlwaysLog(updateFailure, "Super resolution");
        return;
    }
    setProperty("Supported", value);
}

IOReturn IdeaWMISuperRes::setProperties(OSObject *props) {
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &IdeaWMISuperRes::setPropertiesGated), props);
    return kIOReturnSuccess;
}

void IdeaWMISuperRes::setPropertiesGated(OSObject* props) {
    OSDictionary* dict = OSDynamicCast(OSDictionary, props);
    if (!dict)
        return;

    OSCollectionIterator* i = OSCollectionIterator::withCollection(dict);
    if (i) {
        while (OSString* key = OSDynamicCast(OSString, i->getNextObject())) {
            if (key->isEqualTo("ECMonitor")) {
                OSBoolean *value;
                getPropertyBoolean("ECMonitor");
                IOReturn ret;
                OSNumber *in;
                UInt32 result;

                in = OSNumber::withNumber(0ULL, 32);
                ret = YWMI->evaluateInteger(SUPER_RES_DATA_WMI_METHOD, 0,
                                            value->getValue() ? SUPER_RES_DATA_WMI_START : SUPER_RES_DATA_WMI_STOP, &result, in, true);
                OSSafeReleaseNULL(in);

                if (ret != kIOReturnSuccess || result != 1) {
                    AlwaysLog(toggleError, "ECMonitor", ret ?: result);
                } else {
                    DebugLog(toggleSuccess, "ECMonitor", value->getValue() ? SUPER_RES_DATA_WMI_START : SUPER_RES_DATA_WMI_STOP, value->getValue() ? "start" : "stop");
                }
                setProperty("ECMonitor started", value);
            } else {
                AlwaysLog("Unknown property %s", key->getCStringNoCopy());
            }
        }
        i->release();
    }

    return;
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

    if (getGamzeZoneData(GAME_ZONE_WMI_GET_VERSION, &version))
        setProperty("Game Zone Version", version, 32);
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

    ACPIEvent(smartFanModeEvent);
}

void IdeaWMIGameZone::ACPIEvent(UInt32 argument) {
    OSObject *result;
    OSNumber *id;
    if (YWMI->getEventData(argument, &result) && (id = (OSDynamicCast(OSNumber, result)))) {
        if (argument == smartFanModeEvent) {
            switch (id->unsigned32BitValue()) {
                case 1:
                    AlwaysLog("Smart Fan mode: Quiet - Low Speed Fan");
                    break;
                    
                case 2:
                    AlwaysLog("Smart Fan mode: Balance - Balance Acoustic");
                    break;
                    
                case 3:
                    AlwaysLog("Smart Fan mode: Performance - High Speed Fan");
                    break;
                    
                default:
                    DebugLog("Smart Fan mode: unknown - 0x%04x", id->unsigned32BitValue());
                    OSSafeReleaseNULL(result);
                    return;
            }
            smartFanMode = id->unsigned32BitValue() | (version << 16);
            dispatchMessage(kSMC_smartFanEvent, &smartFanMode);
        } else {
            DebugLog("message: ACPI notification 0x%04x - 0x%04x", argument, id->unsigned32BitValue());
        }
    } else {
        AlwaysLog("message: Unknown ACPI notification 0x%04x", argument);
    }
    OSSafeReleaseNULL(result);
}

const char* IdeaWMIGameZone::registerEvent(OSString *guid, UInt32 id) {
    if (guid->isEqualTo(GAME_ZONE_TEMP_EVENT))
        tempEvent = id;
    else if (guid->isEqualTo(GAME_ZONE_OC_EVENT))
        OCEvent = id;
    else if (guid->isEqualTo(GAME_ZONE_GPU_TEMP_EVENT))
        GPUEvent = id;
    else if (guid->isEqualTo(GAME_ZONE_FAN_COOLING_EVENT))
        fanEvent = id;
    else if (guid->isEqualTo(GAME_ZONE_KEYLOCK_STATUS_EVENT))
        keyEvent = id;
    else if (guid->isEqualTo(GAME_ZONE_FAN_MODE_EVENT))
        smartFanModeEvent = id;
    else
        return nullptr;
    return feature;
}
