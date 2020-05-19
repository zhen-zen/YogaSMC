//  SPDX-License-Identifier: GPL-2.0-only
//
//  ThinkWMI.cpp
//  YogaSMC
//
//  Created by Zhen on 6/27/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "ThinkWMI.hpp"

IOService *ThinkWMI::probe(IOService *provider, SInt32 *score)
{
    if (/* DISABLES CODE */ (false)) {
        IOLog("not a valid ThinkPad WMI interface");
        return NULL;
    }
    // TODO: identify an appropritate interface to attach

    return super::probe(provider, score);
}

IOReturn ThinkWMI::setProperties(OSObject *props) {
    if (!vpc)
        return kIOReturnSuccess;

    OSDictionary* dict = OSDynamicCast(OSDictionary, props);
    if (!dict)
        return kIOReturnSuccess;

    OSNumber * batteryValue = OSDynamicCast(OSNumber, dict->getObject(batteryPrompt));
    if (batteryValue == NULL)
        return kIOReturnSuccess;

    switch (batteryValue->unsigned8BitValue()) {
        case BAT_ANY:
            batnum = BAT_ANY;
            break;
        case BAT_PRIMARY:
            batnum = BAT_PRIMARY;
            break;
        case BAT_SECONDARY:
            batnum = BAT_SECONDARY;
            break;

        default:
            IOLog("%s: invalid battery number\n", getName());
            return kIOReturnSuccess;
    }

    IOLog("%s: evaluating properties for battery number %d\n", getName(), batnum);

    updateConservation(getCMstart);
    updateConservation(getCMstop);
    updateConservation(getCMInhibitCharge);
    updateConservation(getCMForceDischarge);
    updateConservation(getCMPeakShiftState);

    return kIOReturnSuccess;
}

bool ThinkWMI::updateConservation(const char * method, bool update) {
    UInt32 result;

    OSObject* params[1] = {
        OSNumber::withNumber(batnum, 32)
    };

    if (vpc->evaluateInteger(method, &result, params, 1) != kIOReturnSuccess) {
        IOLog("%s: %s failed\n", getName(), method);
        return false;
    }

    if (update) {
        IOLog("%s: %s %x\n", getName(), method, result);
        setProperty(method, result);
    }

    return true;
}

bool ThinkWMI::updateMutestatus() {
    vpc->evaluateInteger(getMutestatus, &mutestate);
    IOLog("%s: Mute state %x\n", getName(), mutestate);
    setProperty("Mute", mutestate, 32);
    return true;
}

bool ThinkWMI::initVPC() {
    UInt32 version;

    if (vpc->evaluateInteger(getHKEYversion, &version) != kIOReturnSuccess)
    {
        IOLog("%s: failed to get HKEY interface version\n", getName());
        return false;
    }

    IOLog("%s: HKEY interface version %x\n", getName(), version);
#ifdef DEBUG
    setProperty("HKEY version", version, 32);
#endif
    switch (version >> 8) {
        case 1:
            IOLog("%s: HKEY version 0x100 not implemented\n", getName());
            break;

        case 2:
            updateAdaptiveKBD(1);
            updateAdaptiveKBD(2);
            break;

        default:
            IOLog("%s: Unknown HKEY version\n", getName());
            break;
    }
    updateMutestatus();
    return true;
}

bool ThinkWMI::updateAdaptiveKBD(int arg) {
    UInt32 result;

    OSObject* params[1] = {
        OSNumber::withNumber(arg, 32)
    };

    if (vpc->evaluateInteger(getAdaptiveKBD, &result, params, 1) != kIOReturnSuccess) {
        IOLog("%s: getAdaptiveKBD failed\n", getName());
        return false;
    }

    IOLog("%s: getAdaptiveKBD %d %x\n", getName(), arg, result);
#ifdef DEBUG
    switch (arg) {
        case 1:
            setProperty("hotkey_all_mask", result);
            break;
            
        case 2:
            setProperty("AdaptiveKBD", result != 0);
            break;
            
        default:
            IOLog("%s: Unknown MHKA command\n", getName());
            break;
    }
#endif
    return true;
}
