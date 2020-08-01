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
            IOLog("%s::%s message: ACPI notification 80\n", getName(), name);
            // force enable keyboard and touchpad
            setTopCase(true);
            dispatchMessage(kSMC_FnlockEvent, NULL);
            break;

        case kIOACPIMessageD0:
            if (isYMC) {
                IOLog("%s::%s message: ACPI notification D0\n", getName(), name);
                updateYogaMode();
            } else {
                IOLog("%s::%s message: ACPI notification D0, unknown YMC", getName(), name);
            }
            break;

        default:
            IOLog("%s::%s message: Unknown ACPI notification 0x%x\n", getName(), name, argument);
            break;
    }
    return;
}

IOReturn IdeaWMI::setPowerState(unsigned long powerState, IOService *whatDevice){
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
            IOLog("%s::%s YMC method not found\n", getName(), name);
        }
    }

    if (YWMI->hasMethod(PAPER_LOOKING_WMI_EVENT, ACPI_WMI_EVENT))
        setProperty("Feature", "Paper Display");

    if (YWMI->hasMethod(BAT_INFO_WMI_STRING, ACPI_WMI_EXPENSIVE | ACPI_WMI_STRING)) {
        setProperty("Feature", "WBAT");
        OSArray *BatteryInfo = OSArray::withCapacity(3);
        // only execute once for WMI_EXPENSIVE
        BatteryInfo->setObject(getBatteryInfo(WBAT_BAT0_BatMaker));
        BatteryInfo->setObject(getBatteryInfo(WBAT_BAT0_HwId));
        BatteryInfo->setObject(getBatteryInfo(WBAT_BAT0_MfgDate));
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
    
    if (!YWMI->executeinteger(GSENSOR_DATA_WMI_METHOD, &value, params, 3)) {
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
    dispatchMessage(kSMC_YogaEvent, &YogaMode);
}

void IdeaWMI::stop(IOService *provider) {
    if (YogaMode != kYogaMode_laptop) {
        IOLog("%s::%s Re-enabling top case\n", getName(), name);
        setTopCase(true);
    }
    super::stop(provider);
}

OSString * IdeaWMI::getBatteryInfo(UInt32 index) {
    OSObject* result;

    OSObject* params[1] = {
        OSNumber::withNumber(index, 32),
    };

    if (!YWMI->executeMethod(BAT_INFO_WMI_STRING, &result, params, 1)) {
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

void IdeaWMI::checkEvent(const char *cname, UInt32 id) {
    switch (id) {
        case kIOACPIMessageReserved:
            IOLog("%s::%s found reserved notify id 0x%x for %s\n", getName(), name, id, cname);
            break;
            
        case kIOACPIMessageD0:
            IOLog("%s::%s found YMC notify id 0x%x for %s\n", getName(), name, id, cname);
            break;
            
        default:
            IOLog("%s::%s found unknown notify id 0x%x for %s\n", getName(), name, id, cname);
            break;
    }
}
