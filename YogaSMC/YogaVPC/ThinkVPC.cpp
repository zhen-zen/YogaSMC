//  SPDX-License-Identifier: GPL-2.0-only
//
//  ThinkVPC.cpp
//  YogaSMC
//
//  Created by Zhen on 7/12/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "ThinkVPC.hpp"

OSDefineMetaClassAndStructors(ThinkVPC, YogaVPC);

ThinkVPC* ThinkVPC::withDevice(IOACPIPlatformDevice *device, OSString *pnp) {
    ThinkVPC* vpc = OSTypeAlloc(ThinkVPC);

    OSDictionary* dictionary = OSDictionary::withCapacity(1);
    dictionary->setObject("Type", pnp);

    vpc->vpc = device;

    if (!vpc->init(dictionary) ||
        !vpc->attach(device) ||
        !vpc->start(device)) {
        OSSafeReleaseNULL(vpc);
    }

    dictionary->release();

    return vpc;
}

void ThinkVPC::updateAll() {
    updateBattery(BAT_ANY);
    updateMutestatus();
    updateVPC();
}

bool ThinkVPC::updateConservation(const char * method, bool update) {
    UInt32 result;

    OSObject* params[1] = {
        OSNumber::withNumber(batnum, 32)
    };

    if (vpc->evaluateInteger(method, &result, params, 1) != kIOReturnSuccess) {
        IOLog(updateFailure, getName(), method);
        return false;
    }

    if (update) {
        IOLog(updateSuccess, getName(), method, result);
        setProperty(method, result);
    }

    return true;
}

bool ThinkVPC::updateMutestatus() {
    if (vpc->evaluateInteger(getMutestatus, &mutestate) != kIOReturnSuccess) {
        IOLog(updateFailure, getName(), __func__);
        return false;
    }
    IOLog(updateSuccess, getName(), __func__, mutestate);
    setProperty(mutePrompt, mutestate, 32);
    return true;
}

bool ThinkVPC::initVPC() {
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
    updateAll();
    return true;
}

bool ThinkVPC::updateAdaptiveKBD(int arg) {
    UInt32 result;

    OSObject* params[1] = {
        OSNumber::withNumber(arg, 32)
    };

    if (vpc->evaluateInteger(getAdaptiveKBD, &result, params, 1) != kIOReturnSuccess) {
        IOLog(updateFailure, getName(), __func__);
        return false;
    }

    IOLog("%s: %s %d %x\n", getName(), __func__, arg, result);
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

void ThinkVPC::updateBattery(int battery) {
    switch (battery) {
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
            IOLog(valueInvalid, getName(), batteryPrompt);
            return;
    }
    
    IOLog(updateSuccess, getName(), batteryPrompt, batnum);
    
    updateConservation(getCMstart);
    updateConservation(getCMstop);
    updateConservation(getCMInhibitCharge);
    updateConservation(getCMForceDischarge);
    updateConservation(getCMPeakShiftState);
}

void ThinkVPC::setPropertiesGated(OSObject *props) {
    if (!vpc) {
        IOLog(VPCUnavailable, getName());
        return;
    }

    OSDictionary* dict = OSDynamicCast(OSDictionary, props);
    if (!dict)
        return;

    //    IOLog("%s: %d objects in properties\n", getName(), dict->getCount());
    OSCollectionIterator* i = OSCollectionIterator::withCollection(dict);

    if (i != NULL) {
        while (OSString* key = OSDynamicCast(OSString, i->getNextObject())) {
            if (key->isEqualTo(batteryPrompt)) {
                OSNumber * value = OSDynamicCast(OSNumber, dict->getObject(batteryPrompt));
                if (value == NULL) {
                    IOLog(valueInvalid, getName(), batteryPrompt);
                    continue;
                }

                updateBattery(value->unsigned8BitValue());
            } else if (key->isEqualTo(mutePrompt)) {
                updateMutestatus();
            } else {
                IOLog("%s: Unknown property %s\n", getName(), key->getCStringNoCopy());
            }
        }
        i->release();
    }

    return;
}
