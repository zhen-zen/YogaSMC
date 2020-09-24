//  SPDX-License-Identifier: GPL-2.0-only
//
//  IdeaWMI.cpp
//  YogaSMC
//
//  Created by Zhen on 2020/6/23.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "IdeaWMI.hpp"
OSDefineMetaClassAndStructors(IdeaWMI, YogaWMI);

void IdeaWMI::ACPIEvent(UInt32 argument) {
    switch (argument) {
        case kIOACPIMessageReserved:
            AlwaysLog("message: ACPI notification 80");
            // force enable keyboard and touchpad
            setTopCase(true);
            dispatchMessage(kSMC_FnlockEvent, NULL);
            break;

        case kIOACPIMessageD0:
            if (isYMC) {
                AlwaysLog("message: ACPI notification D0");
                updateYogaMode();
            } else {
                AlwaysLog("message: ACPI notification D0, unknown YMC");
            }
            break;

        default:
            AlwaysLog("message: Unknown ACPI notification 0x%x", argument);
            break;
    }
    return;
}

IOReturn IdeaWMI::setPowerState(unsigned long powerState, IOService *whatDevice){
    AlwaysLog("powerState %ld : %s", powerState, powerState ? "on" : "off");

    if (whatDevice != this)
        return kIOReturnInvalid;

    if (isYMC) {
        if (powerState == 0) {
            if (YogaMode != kYogaMode_laptop) {
                AlwaysLog("Re-enabling top case");
                setTopCase(true);
            }
        } else {
            updateYogaMode();
        }
    }
    return kIOPMAckImplied;
}

void IdeaWMI::processWMI() {
    if (YWMI->hasMethod(GSENSOR_WMI_EVENT, ACPI_WMI_EVENT)) {
        if (YWMI->hasMethod(GSENSOR_DATA_WMI_METHOD)) {
            setProperty("Feature", "Yoga Mode Control");
            isYMC = true;
        } else {
            AlwaysLog("YMC method not found");
        }
    }

    if (YWMI->hasMethod(PAPER_LOOKING_WMI_EVENT, ACPI_WMI_EVENT))
        setProperty("Feature", "Paper Display");

    if (YWMI->hasMethod(BAT_INFO_WMI_STRING, ACPI_WMI_EXPENSIVE | ACPI_WMI_STRING)) {
        setProperty("Feature", "WBAT");
        OSArray *BatteryInfo = OSArray::withCapacity(3);
        // only execute once for WMI_EXPENSIVE
        OSObject *value;
        value = OSString::withCString(getBatteryInfo(WBAT_BAT0_BatMaker));
        BatteryInfo->setObject(value);
        value->release();
        value = OSString::withCString(getBatteryInfo(WBAT_BAT0_HwId));
        BatteryInfo->setObject(value);
        value->release();
        value = OSString::withCString(getBatteryInfo(WBAT_BAT0_MfgDate));
        BatteryInfo->setObject(value);
        value->release();
        setProperty("BatteryInfo", BatteryInfo);
        BatteryInfo->release();
    }
}

void IdeaWMI::updateYogaMode() {
    OSObject* params[3] = {
        OSNumber::withNumber(0ULL, 32),
        OSNumber::withNumber(0ULL, 32),
        OSNumber::withNumber(0ULL, 32)
    };

    UInt32 value;
    
    if (!YWMI->executeInteger(GSENSOR_DATA_WMI_METHOD, &value, params, 3)) {
        setProperty("YogaMode", false);
        AlwaysLog("YogaMode: detection failed");
        return;
    }

    AlwaysLog("YogaMode: %d", value);
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
            AlwaysLog("Unknown yoga mode: %d", value);
            setProperty("YogaMode", value, 32);
            return;
    }
    YogaMode = value;
    dispatchMessage(kSMC_YogaEvent, &YogaMode);
}

void IdeaWMI::stop(IOService *provider) {
    if (YogaMode != kYogaMode_laptop) {
        AlwaysLog("Re-enabling top case");
        setTopCase(true);
    }
    super::stop(provider);
}

const char *IdeaWMI::getBatteryInfo(UInt32 index) {
    OSObject* result;

    OSObject* params[1] = {
        OSNumber::withNumber(index, 32),
    };

    if (!YWMI->executeMethod(BAT_INFO_WMI_STRING, &result, params, 1)) {
        AlwaysLog("WBAT evaluation failed");
        return "evaluation failed";
    }

    OSString *info = OSDynamicCast(OSString, result);

    if (!info) {
        AlwaysLog("WBAT result not a string");
        return "result not a string";
    }
    AlwaysLog("WBAT %s", info->getCStringNoCopy());
    return info->getCStringNoCopy();
}

void IdeaWMI::checkEvent(const char *cname, UInt32 id) {
    switch (id) {
        case kIOACPIMessageReserved:
            AlwaysLog("found reserved notify id 0x%x for %s", id, cname);
            break;
            
        case kIOACPIMessageD0:
            AlwaysLog("found YMC notify id 0x%x for %s", id, cname);
            break;
            
        default:
            AlwaysLog("found unknown notify id 0x%x for %s", id, cname);
            break;
    }
}
