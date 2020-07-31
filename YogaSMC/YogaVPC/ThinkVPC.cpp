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
    dictionary->setObject("Feature", pnp);

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
        IOLog(updateFailure, getName(), name, method);
        return false;
    }

    if (update) {
        IOLog(updateSuccess, getName(), name, method, result);
        setProperty(method, result, 32);
    }

    return true;
}

bool ThinkVPC::setConservation(const char * method, UInt32 value) {
    UInt32 result;

    OSObject* params[1] = {
        OSNumber::withNumber(value, 32)
    };

    if (vpc->evaluateInteger(method, &result, params, 1) != kIOReturnSuccess) {
        IOLog(updateFailure, getName(), name, method);
        return false;
    }

    IOLog(updateSuccess, getName(), name, method, result);
    updateBattery(batnum);

    return true;
}

bool ThinkVPC::updateMutestatus() {
    if (vpc->evaluateInteger(getMutestatus, &mutestate) != kIOReturnSuccess) {
        IOLog(updateFailure, getName(), name, __func__);
        return false;
    }
    IOLog(updateSuccess, getName(), name, __func__, mutestate);
    setProperty(mutePrompt, mutestate, 32);
    return true;
}

bool ThinkVPC::initVPC() {
    UInt32 version;

    if (vpc->evaluateInteger(getHKEYversion, &version) != kIOReturnSuccess)
    {
        IOLog(initFailure, getName(), name, getHKEYversion);
        return false;
    }

    IOLog("%s::%s HKEY interface version %x\n", getName(), name, version);
#ifdef DEBUG
    setProperty("HKEY version", version, 32);
#endif
    switch (version >> 8) {
        case 1:
            IOLog("%s::%s HKEY version 0x100 not implemented\n", getName(), name);
            break;

        case 2:
            updateAdaptiveKBD(1);
            updateAdaptiveKBD(2);
            break;

        default:
            IOLog("%s::%s Unknown HKEY version\n", getName(), name);
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
        IOLog(updateFailure, getName(), name, __func__);
        return false;
    }

    IOLog("%s::%s %s %d %x\n", getName(), name, __func__, arg, result);
#ifdef DEBUG
    switch (arg) {
        case 1:
            setProperty("hotkey_all_mask", result);
            break;
            
        case 2:
            setProperty("AdaptiveKBD", result != 0);
            break;
            
        default:
            IOLog("%s::%s Unknown MHKA command\n", getName(), name);
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
            IOLog(valueInvalid, getName(), name, batteryPrompt);
            return;
    }
    
    IOLog(updateSuccess, getName(), name, batteryPrompt, batnum);
    
    updateConservation(getCMstart);
    updateConservation(getCMstop);
    updateConservation(getCMInhibitCharge);
    updateConservation(getCMForceDischarge);
    updateConservation(getCMPeakShiftState);
}

void ThinkVPC::setPropertiesGated(OSObject *props) {
    if (!vpc) {
        IOLog(VPCUnavailable, getName(), name);
        return;
    }

    OSDictionary* dict = OSDynamicCast(OSDictionary, props);
    if (!dict)
        return;

    //    IOLog("%s::%s %d objects in properties\n", getName(), name, dict->getCount());
    OSCollectionIterator* i = OSCollectionIterator::withCollection(dict);

    if (i) {
        while (OSString* key = OSDynamicCast(OSString, i->getNextObject())) {
            if (key->isEqualTo(batteryPrompt)) {
                OSNumber * value = OSDynamicCast(OSNumber, dict->getObject(batteryPrompt));
                if (value == nullptr) {
                    IOLog(valueInvalid, getName(), name, batteryPrompt);
                    continue;
                }

                updateBattery(value->unsigned8BitValue());
            } else if (key->isEqualTo("setCMstart")) {
                OSNumber * value = OSDynamicCast(OSNumber, dict->getObject("setCMstart"));
                if (value == nullptr || value->unsigned32BitValue() > 100) {
                    IOLog(valueInvalid, getName(), name, "setCMstart");
                    continue;
                }

                setConservation(setCMstart, value->unsigned8BitValue());
            } else if (key->isEqualTo("setCMstop")) {
                OSNumber * value = OSDynamicCast(OSNumber, dict->getObject("setCMstop"));
                if (value == nullptr || value->unsigned32BitValue() > 100) {
                    IOLog(valueInvalid, getName(), name, "setCMstop");
                    continue;
                }

                setConservation(setCMstop, value->unsigned8BitValue());
            } else if (key->isEqualTo(mutePrompt)) {
                updateMutestatus();
            } else if (key->isEqualTo(fanControlPrompt)) {
                OSNumber * value = OSDynamicCast(OSNumber, dict->getObject(fanControlPrompt));
                if (value == nullptr) {
                    IOLog(valueInvalid, getName(), name, FnKeyPrompt);
                    continue;
                }
                setFanControl(value->unsigned32BitValue());
            } else {
                IOLog("%s::%s Unknown property %s\n", getName(), name, key->getCStringNoCopy());
            }
        }
        i->release();
    }

    return;
}

bool ThinkVPC::setFanControl(int level) {
    UInt32 result;

    OSObject* params[1] = {
        OSNumber::withNumber((level ? BIT(18) : 0), 32)
    };

    if (vpc->evaluateInteger(setControl, &result, params, 1) != kIOReturnSuccess) {
        IOLog(toggleFailure, getName(), name, fanControlPrompt);
        return false;
    }
    if (!result) {
        IOLog(toggleFailure, getName(), name, "FanControl0");
        return false;
    }

    IOLog(toggleSuccess, getName(), name, fanControlPrompt, (level ? BIT(18) : 0), (level ? "7" : "auto"));
    setProperty(fanControlPrompt, level, 32);
    return true;
}
