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
    updateMuteLEDStatus();
    updateMicMuteLEDStatus();
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

bool ThinkVPC::updateMutestatus(bool update) {
    if (vpc->evaluateInteger(getAudioMutestatus, &mutestate) != kIOReturnSuccess) {
        IOLog(updateFailure, getName(), name, __func__);
        return false;
    }

    if (update)
        IOLog(updateSuccess, getName(), name, __func__, mutestate);

    setProperty(mutePrompt, mutestate, 32);
    return true;
}

bool ThinkVPC::setMutestatus(UInt32 value) {
    updateMutestatus(false);
    if (mutestate == value) {
        IOLog(valueMatched, getName(), name, mutePrompt, mutestate);
        return true;
    }

    UInt32 result;

    OSObject* params[1] = {
        OSNumber::withNumber(value, 32)
    };

    if (vpc->evaluateInteger(setAudioMutestatus, &result, params, 1) != kIOReturnSuccess || result != value) {
        IOLog(toggleFailure, getName(), name, __func__);
        return false;
    }

    IOLog(toggleSuccess, getName(), name, __func__, mutestate, (mutestate == 2 ? "SW" : "HW"));
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

    mutestate_orig = mutestate;
    IOLog("%s::%s Initial HAUM setting was %d", getName(), name, mutestate_orig);
    setMutestatus(TP_EC_MUTE_BTN_NONE);

    return true;
}

bool ThinkVPC::exitVPC() {
    setMutestatus(mutestate_orig);
    return super::exitVPC();
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
                OSNumber * value = OSDynamicCast(OSNumber, dict->getObject(mutePrompt));
                if (value == nullptr || value->unsigned32BitValue() > 3) {
                    IOLog(valueInvalid, getName(), name, "mutePrompt");
                    continue;
                }

                updateMutestatus(false);

                setMutestatus(value->unsigned32BitValue());
            } else if (key->isEqualTo(muteLEDPrompt)) {
                OSBoolean * value = OSDynamicCast(OSBoolean, dict->getObject(muteLEDPrompt));
                if (value == nullptr) {
                    IOLog(valueInvalid, getName(), name, muteLEDPrompt);
                    continue;
                }

                setMuteLEDStatus(value->getValue());
            } else if (key->isEqualTo(micMuteLEDPrompt)) {
                OSNumber * value = OSDynamicCast(OSNumber, dict->getObject(micMuteLEDPrompt));
                if (value == nullptr) {
                    IOLog(valueInvalid, getName(), name, micMuteLEDPrompt);
                    continue;
                }

                setMicMuteLEDStatus(value->unsigned32BitValue());
            } else if (key->isEqualTo(muteSupportPrompt)) {
                OSBoolean * value = OSDynamicCast(OSBoolean, dict->getObject(muteSupportPrompt));
                if (value == nullptr) {
                    IOLog(valueInvalid, getName(), name, muteSupportPrompt);
                    continue;
                }

                setMuteSupport(value->getValue());
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

bool ThinkVPC::updateMuteLEDStatus(bool update) {
    if (vpc->evaluateInteger(getAudioMuteLED, &muteLEDstate) != kIOReturnSuccess) {
        IOLog(updateFailure, getName(), name, __func__);
        return false;
    }

    if (update)
        IOLog(updateSuccess, getName(), name, __func__, muteLEDstate);

    setProperty(muteLEDPrompt, muteLEDstate, 32);
    return true;
}

bool ThinkVPC::setMuteSupport(bool support) {
    UInt32 result;

    OSObject* params[1] = {
        OSNumber::withNumber(support, 32)
    };

    if (vpc->evaluateInteger(setAudioMuteSupport, &result, params, 1) != kIOReturnSuccess || result & TPACPI_AML_MUTE_ERROR_STATE_MASK) {
        IOLog(toggleFailure, getName(), name, muteSupportPrompt);
        return false;
    }

    IOLog(toggleSuccess, getName(), name, muteSupportPrompt, support, (support ? "disable" : "enable"));
    setProperty(muteSupportPrompt, support);
    return true;
}

bool ThinkVPC::setMuteLEDStatus(bool status) {
    UInt32 result;

    OSObject* params[1] = {
        OSNumber::withNumber(status, 32)
    };

    if (vpc->evaluateInteger(setAudioMuteLED, &result, params, 1) != kIOReturnSuccess) {
        IOLog(toggleFailure, getName(), name, muteLEDPrompt);
        return false;
    }

    IOLog(toggleSuccess, getName(), name, muteLEDPrompt, status, (status ? "on" : "off"));
    setProperty(muteLEDPrompt, status);
    return true;
}

bool ThinkVPC::updateMicMuteLEDStatus(bool update) {
    if (vpc->evaluateInteger(getMicMuteLED, &micMuteLEDstate) != kIOReturnSuccess) {
        IOLog(updateFailure, getName(), name, __func__);
        return false;
    }

    if (update)
        IOLog(updateSuccess, getName(), name, __func__, micMuteLEDstate);

    if (micMuteLEDstate & TPACPI_AML_MIC_MUTE_HDMC)
        setProperty(micMuteLEDPrompt, "HDMC");
    else
        setProperty(micMuteLEDPrompt, micMuteLEDstate, 32);
    return true;
}

bool ThinkVPC::setMicMuteLEDStatus(UInt32 status) {
    UInt32 result;

    OSObject* params[1] = {
        OSNumber::withNumber(status, 32)
    };

    if (vpc->evaluateInteger(setMicMuteLED, &result, params, 1) != kIOReturnSuccess) {
        IOLog(toggleFailure, getName(), name, micMuteLEDPrompt);
        return false;
    }

    IOLog(toggleSuccess, getName(), name, micMuteLEDPrompt, status, (status ? "on / blink" : "off"));
    setProperty(micMuteLEDPrompt, status, 32);
    return true;
}
