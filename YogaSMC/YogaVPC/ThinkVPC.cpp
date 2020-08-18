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
    getNotificationMask(1);
    getNotificationMask(2);
    getNotificationMask(3);
    updateBattery(BAT_ANY);
    updateMutestatus();
    updateMuteLEDStatus();
    updateMicMuteLEDStatus();
    updateVPC();
}

bool ThinkVPC::updateConservation(const char * method, bool update) {
    UInt32 result;

    OSObject* params[] = {
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

    OSObject* params[] = {
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
    if (ec->evaluateInteger(getAudioMutestatus, &mutestate) != kIOReturnSuccess) {
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

    OSObject* params[] = {
        OSNumber::withNumber(value, 32)
    };

    if (vpc->evaluateInteger(setAudioMutestatus, &result, params, 1) != kIOReturnSuccess) {
        IOLog(toggleFailure, getName(), name, __func__);
        return false;
    }

    IOLog(toggleSuccess, getName(), name, __func__, mutestate, (mutestate == 2 ? "SW" : "HW"));
    setProperty(mutePrompt, mutestate, 32);
    return true;
}

void ThinkVPC::getNotificationMask(UInt32 index) {
    UInt32 result;

    OSObject* params[] = {
        OSNumber::withNumber(index, 32),
    };

    if (vpc->evaluateInteger(getHKEYmask, &result, params, 1) != kIOReturnSuccess) {
        IOLog(toggleFailure, getName(), name, __func__);
        return;
    }

    switch (index) {
        case 1:
            setProperty("DHKN", result, 32);
            break;

        case 2:
            setProperty("DHKE", result, 32);
            break;

        case 3:
            setProperty("DHKF", result, 32);
            break;

        default:
            break;
    }
}

IOReturn ThinkVPC::setNotificationMask(UInt32 i, UInt32 all_mask, UInt32 offset, bool enable) {
    IOReturn ret = kIOReturnSuccess;
    OSObject* params[] = {
        OSNumber::withNumber(i + offset + 1, 32),
        OSNumber::withNumber(enable, 32),
    };
    if (all_mask & BIT(i))
        ret = vpc->evaluateObject(setHKEYmask, nullptr, params, 1);
    params[0]->release();
    params[1]->release();
    return ret;
}

bool ThinkVPC::initVPC() {
    super::initVPC();
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
            updateAdaptiveKBD(0);
            break;

        case 2:
            updateAdaptiveKBD(1);
            updateAdaptiveKBD(2);
            updateAdaptiveKBD(3);
            break;

        default:
            IOLog("%s::%s Unknown HKEY version\n", getName(), name);
            break;
    }

    if (!hotkey_all_mask) {
        IOLog("%s::%s Failed to acquire hotkey_all_mask\n", getName(), name);
    } else {
        for (UInt32 i = 0; i < 0x20; i++) {
            if (setNotificationMask(i, hotkey_all_mask, DHKN_MASK_offset) != kIOReturnSuccess)
                break;
        }
    }

    updateAll();

    mutestate_orig = mutestate;
    IOLog("%s::%s Initial HAUM setting was %d\n", getName(), name, mutestate_orig);
    setMutestatus(TP_EC_MUTE_BTN_NONE);
    setHotkeyStatus(true);

    UInt32 state;
    OSObject* params[] = {
        OSNumber::withNumber(0ULL, 32)
    };

    if (vpc->evaluateInteger(getKBDBacklightLevel, &state, params, 1) == kIOReturnSuccess)
        backlightCap = BIT(KBD_BACKLIGHT_CAP_BIT) & state;

    if (!backlightCap)
        setProperty(backlightPrompt, "unsupported");

    return true;
}

bool ThinkVPC::exitVPC() {
    setHotkeyStatus(false);
    setMutestatus(mutestate_orig);
    return super::exitVPC();
}

bool ThinkVPC::updateAdaptiveKBD(int arg) {
    UInt32 result;

    if (!arg) {
        if (vpc->evaluateInteger(getHKEYAdaptive, &result) != kIOReturnSuccess) {
            IOLog(updateFailure, getName(), name, __func__);
            return false;
        }
    } else {
        OSObject* params[] = {
            OSNumber::withNumber(arg, 32)
        };

        if (vpc->evaluateInteger(getHKEYAdaptive, &result, params, 1) != kIOReturnSuccess) {
            IOLog(updateFailure, getName(), name, __func__);
            return false;
        }
    }

    IOLog("%s::%s %s %d %x\n", getName(), name, __func__, arg, result);
    switch (arg) {
        case 0:
        case 1:
            hotkey_all_mask = result;
            setProperty("hotkey_all_mask", result, 32);
            break;

        case 2:
            if (result) {
                hotkey_adaptive_all_mask = result;
                setProperty("hotkey_adaptive_all_mask", result, 32);
            }
            break;

        case 3:
            hotkey_alter_all_mask = result;
            setProperty("hotkey_alter_all_mask", result, 32);
            break;

        default:
            IOLog("%s::%s Unknown MHKA command\n", getName(), name);
            break;
    }
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
                OSDictionary* entry = OSDictionary::withCapacity(1);
                entry->setObject(key, dict->getObject(key));
                super::setPropertiesGated(entry);
                entry->release();
            }
        }
        i->release();
    }

    return;
}

bool ThinkVPC::setFanControl(int level) {
    UInt32 result;

    OSObject* params[] = {
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
    OSObject* params[] = {
        OSNumber::withNumber(0ULL, 32)
    };

    if (vpc->evaluateInteger(getAudioMuteLED, &muteLEDstate, params, 1) != kIOReturnSuccess) {
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

    OSObject* params[] = {
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

    OSObject* params[] = {
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

    OSObject* params[] = {
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

IOReturn ThinkVPC::message(UInt32 type, IOService *provider, void *argument) {
    switch (type)
    {
        case kSMC_setDisableTouchpad:
        case kSMC_getDisableTouchpad:
        case kPS2M_notifyKeyPressed:
        case kPS2M_notifyKeyTime:
        case kPS2M_resetTouchpad:
        case kSMC_setKeyboardStatus:
        case kSMC_getKeyboardStatus:
            break;

        case kSMC_FnlockEvent:
            break;

        // TODO: determine default value for charge level
        case kSMC_getConservation:
        case kSMC_setConservation:
            break;

        case kIOACPIMessageDeviceNotification:
            if (!argument)
                IOLog("%s::%s message: Unknown ACPI notification\n", getName(), name);
            else if (*((UInt32 *) argument) == kIOACPIMessageReserved)
                updateVPC();
            else
                IOLog("%s::%s message: Unknown ACPI notification 0x%04x\n", getName(), name, *((UInt32 *) argument));
            break;

        default:
            if (argument)
                IOLog("%s::%s message: type=%x, provider=%s, argument=0x%04x\n", getName(), name, type, provider->getName(), *((UInt32 *) argument));
            else
                IOLog("%s::%s message: type=%x, provider=%s\n", getName(), name, type, provider->getName());
    }

    return kIOReturnSuccess;
}

void ThinkVPC::updateVPC() {
    UInt32 result;
    if (vpc->evaluateInteger(getHKEYevent, &result) != kIOReturnSuccess) {
        IOLog(toggleFailure, getName(), name, HotKeyPrompt);
        return;
    }

    if (!result) {
        IOLog("%s::%s empty HKEY event", getName(), name);
        return;
    }

    switch (result >> 0xC) {
        case 1:
            IOLog("%s::%s Hotkey(MHKP) key presses event: 0x%x \n", getName(), name, result);
            break;

        case 2:
            IOLog("%s::%s Hotkey(MHKP) wakeup reason event: 0x%x \n", getName(), name, result);
            break;

        case 3:
            IOLog("%s::%s Hotkey(MHKP) bay-related wakeup event: 0x%x \n", getName(), name, result);
            break;

        case 4:
            IOLog("%s::%s Hotkey(MHKP) dock-related event: 0x%x \n", getName(), name, result);
            break;

        case 5:
            switch (result) {
                case TP_HKEY_EV_PEN_INSERTED:
                    IOLog("%s::%s tablet pen inserted into bay\n", getName(), name);
                    break;

                case TP_HKEY_EV_PEN_REMOVED:
                    IOLog("%s::%s tablet pen removed from bay\n", getName(), name);
                    break;

                case TP_HKEY_EV_TABLET_TABLET:
                    IOLog("%s::%s tablet mode\n", getName(), name);
                    break;

                case TP_HKEY_EV_TABLET_NOTEBOOK:
                    IOLog("%s::%s normal mode\n", getName(), name);
                    break;

                case TP_HKEY_EV_LID_CLOSE:
                    IOLog("%s::%s Lid closed\n", getName(), name);
                    break;

                case TP_HKEY_EV_LID_OPEN:
                    IOLog("%s::%s Lid opened\n", getName(), name);
                    break;

                case TP_HKEY_EV_BRGHT_CHANGED:
                    IOLog("%s::%s brightness changed\n", getName(), name);
                    break;

                default:
                    IOLog("%s::%s Hotkey(MHKP) unknown human interface event: 0x%x \n", getName(), name, result);
                    break;
            }
            break;

        case 6:
            switch (result) {
                case TP_HKEY_EV_THM_TABLE_CHANGED:
                    IOLog("%s::%s Thermal Table has changed\n", getName(), name);
                    break;

                case TP_HKEY_EV_THM_CSM_COMPLETED:
                    IOLog("%s::%s Thermal Control Command set completed (DYTC)\n", getName(), name);
                    UInt64 output;
                    if (DYTCCap && setDYTCMode(DYTC_CMD_GET, &output)) {
                        DYTCMode = output;
                        setProperty("DYTCMode", output, 64);
                        IOLog("%s::%s DYTC lapmode %s\n", getName(), name, (output & BIT(DYTC_GET_LAPMODE_BIT) ? "on" : "off"));
                    }
                    break;

                case TP_HKEY_EV_THM_TRANSFM_CHANGED:
                    IOLog("%s::%s Thermal Transformation changed (GMTS)\n", getName(), name);
                    break;

                case TP_HKEY_EV_ALARM_BAT_HOT:
                    IOLog("%s::%s THERMAL ALARM: battery is too hot!\n", getName(), name);
                    break;

                case TP_HKEY_EV_ALARM_BAT_XHOT:
                    IOLog("%s::%s THERMAL EMERGENCY: battery is extremely hot!\n", getName(), name);
                    break;

                case TP_HKEY_EV_ALARM_SENSOR_HOT:
                    IOLog("%s::%s THERMAL ALARM: a sensor reports something is too hot!\n", getName(), name);
                    break;

                case TP_HKEY_EV_ALARM_SENSOR_XHOT:
                    IOLog("%s::%s THERMAL EMERGENCY: a sensor reports something is extremely hot!\n", getName(), name);
                    break;

                case TP_HKEY_EV_AC_CHANGED:
                    IOLog("%s::%s AC status changed\n", getName(), name);
                    break;

                case TP_HKEY_EV_KEY_NUMLOCK:
                    IOLog("%s::%s Numlock\n", getName(), name);
                    break;

                case TP_HKEY_EV_KEY_FN:
                    IOLog("%s::%s Fn\n", getName(), name);
                    break;

                case TP_HKEY_EV_KEY_FN_ESC:
                    IOLog("%s::%s Fn+Esc\n", getName(), name);
                    break;

                case TP_HKEY_EV_TABLET_CHANGED:
                    IOLog("%s::%s tablet mode has changed\n", getName(), name);
                    break;

                case TP_HKEY_EV_PALM_DETECTED:
                    IOLog("%s::%s palm detected hovering the keyboard\n", getName(), name);
                    break;

                case TP_HKEY_EV_PALM_UNDETECTED:
                    IOLog("%s::%s palm undetected hovering the keyboard\n", getName(), name);
                    break;

                default:
                    IOLog("%s::%s Hotkey(MHKP) unknown thermal alarms/notices and keyboard event: 0x%x \n", getName(), name, result);
                    break;
            }
            break;

        case 7:
            IOLog("%s::%s Hotkey(MHKP) misc event: 0x%x \n", getName(), name, result);
            break;

        default:
            IOLog("%s::%s Hotkey(MHKP) unknown event: 0x%x \n", getName(), name, result);
            break;
    }
}

bool ThinkVPC::setHotkeyStatus(bool enable) {
    UInt32 result;

    OSObject* params[] = {
        OSNumber::withNumber(enable, 32)
    };

    if (vpc->evaluateInteger(setHKEYstatus, &result, params, 1) != kIOReturnSuccess) {
        IOLog(toggleFailure, getName(), name, HotKeyPrompt);
        return false;
    }

    IOLog(toggleSuccess, getName(), name, HotKeyPrompt, enable, (enable ? "enabled" : "disabled"));
    setProperty(HotKeyPrompt, enable);
    return true;
}

bool ThinkVPC::updateBacklight(bool update) {
    if (!backlightCap)
        return true;

    UInt32 state;
    OSObject* params[] = {
        OSNumber::withNumber(0ULL, 32)
    };

    if (vpc->evaluateInteger(getKBDBacklightLevel, &state, params, 1) != kIOReturnSuccess) {
        IOLog(updateFailure, getName(), name, KeyboardPrompt);
        return false;
    }

    backlightLevel = state & 0x3;

    if (update) {
        IOLog(updateSuccess, getName(), name, KeyboardPrompt, state);
        if (backlightCap)
            setProperty(backlightPrompt, backlightLevel, 32);
    }

    return true;
}

bool ThinkVPC::setBacklight(UInt32 level) {
    if (!backlightCap)
        return true;

    UInt32 result;

    OSObject* params[1] = {
        OSNumber::withNumber(level, 32)
    };

    if (vpc->evaluateInteger(setKBDBacklightLevel, &result, params, 1) != kIOReturnSuccess || result != 0) {
        IOLog(toggleFailure, getName(), name, backlightPrompt);
        return false;
    }

    backlightLevel = level;
    IOLog(toggleSuccess, getName(), name, backlightPrompt, backlightLevel, (backlightLevel ? "on" : "off"));
    setProperty(backlightPrompt, backlightLevel, 32);

    return true;
}
