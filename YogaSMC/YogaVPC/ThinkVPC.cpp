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

void ThinkVPC::updateAll() {
    getNotificationMask(1);
    getNotificationMask(2);
    getNotificationMask(3);
    updateBattery(BAT_ANY);
    updateMutestatus();
    updateMuteLEDStatus();
    updateMicMuteLEDStatus();
    updateVPC();
    setMicMuteLEDStatus(0);
}

bool ThinkVPC::updateConservation(const char * method, bool update) {
    UInt32 result;

    OSObject* params[] = {
        OSNumber::withNumber(batnum, 32)
    };

    if (vpc->evaluateInteger(method, &result, params, 1) != kIOReturnSuccess) {
        AlwaysLog(updateFailure, method);
        return false;
    }

    if (update) {
        DebugLog(updateSuccess, method, result);
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
        AlwaysLog(updateFailure, method);
        return false;
    }

    DebugLog(updateSuccess, method, result);
    updateBattery();

    return true;
}

bool ThinkVPC::updateMutestatus(bool update) {
    if (ec->evaluateInteger(getAudioMutestatus, &mutestate) != kIOReturnSuccess) {
        AlwaysLog(updateFailure, __func__);
        return false;
    }

    if (update)
        DebugLog(updateSuccess, __func__, mutestate);

    setProperty(mutePrompt, mutestate, 32);
    return true;
}

bool ThinkVPC::setMutestatus(UInt32 value) {
    updateMutestatus(false);
    if (mutestate == value) {
        DebugLog(valueMatched, mutePrompt, mutestate);
        return true;
    }

    UInt32 result;

    OSObject* params[] = {
        OSNumber::withNumber(value, 32)
    };

    if (vpc->evaluateInteger(setAudioMutestatus, &result, params, 1) != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, __func__);
        return false;
    }

    DebugLog(toggleSuccess, __func__, mutestate, (mutestate == 2 ? "SW" : "HW"));
    setProperty(mutePrompt, mutestate, 32);
    return true;
}

void ThinkVPC::getNotificationMask(UInt32 index) {
    UInt32 result;

    OSObject* params[] = {
        OSNumber::withNumber(index, 32),
    };

    if (vpc->evaluateInteger(getHKEYmask, &result, params, 1) != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, __func__);
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
    if (all_mask & BIT(i)) {
        ret = vpc->evaluateObject(setHKEYmask, nullptr, params, 2);
        AlwaysLog("setting HKEY mask BIT %x\n", i);
    }
    params[0]->release();
    params[1]->release();
    return ret;
}

bool ThinkVPC::initVPC() {
    super::initVPC();
    UInt32 version;

    if (vpc->evaluateInteger(getHKEYversion, &version) != kIOReturnSuccess)
    {
        AlwaysLog(initFailure, getHKEYversion);
        return false;
    }

    AlwaysLog("HKEY interface version %x\n", version);
#ifdef DEBUG
    setProperty("HKEY version", version, 32);
#endif
    switch (version >> 8) {
        case 1:
            AlwaysLog("HKEY version 0x100 not implemented\n");
            updateAdaptiveKBD(0);
            break;

        case 2:
            updateAdaptiveKBD(1);
            updateAdaptiveKBD(2);
            updateAdaptiveKBD(3);
            break;

        default:
            AlwaysLog("Unknown HKEY version\n");
            break;
    }

    if (hotkey_all_mask) {
        for (UInt32 i = 0; i < 0x20; i++) {
            if (setNotificationMask(i, hotkey_all_mask, DHKN_MASK_offset) != kIOReturnSuccess)
                break;
        }
    } else {
        AlwaysLog("Failed to acquire hotkey_all_mask\n");
    }

    updateAll();

    mutestate_orig = mutestate;
    AlwaysLog("Initial HAUM setting was %d\n", mutestate_orig);
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
            AlwaysLog(updateFailure, __func__);
            return false;
        }
    } else {
        OSObject* params[] = {
            OSNumber::withNumber(arg, 32)
        };

        if (vpc->evaluateInteger(getHKEYAdaptive, &result, params, 1) != kIOReturnSuccess) {
            AlwaysLog(updateFailure, __func__);
            return false;
        }
    }

    AlwaysLog("%s %d %x\n", __func__, arg, result);
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
            AlwaysLog("Unknown MHKA command\n");
            break;
    }
    return true;
}

void ThinkVPC::updateBattery(bool update) {
    switch (batnum) {
        case BAT_ANY:
        case BAT_PRIMARY:
        case BAT_SECONDARY:
            break;

        default:
            batnum = BAT_PRIMARY;
            AlwaysLog(valueInvalid, batteryPrompt);
            return;
    }

    if (updateConservation(getCMstart) &&
        updateConservation(getCMstop) &&
        updateConservation(getCMInhibitCharge) &&
        updateConservation(getCMForceDischarge) &&
        updateConservation(getCMPeakShiftState))
        DebugLog(updateSuccess, batteryPrompt, batnum);
    else
        AlwaysLog(updateFailure, batteryPrompt);
}

void ThinkVPC::setPropertiesGated(OSObject *props) {
    OSDictionary* dict = OSDynamicCast(OSDictionary, props);
    if (!dict)
        return;

    //    AlwaysLog("%d objects in properties\n", dict->getCount());
    OSCollectionIterator* i = OSCollectionIterator::withCollection(dict);

    if (i) {
        while (OSString* key = OSDynamicCast(OSString, i->getNextObject())) {
            if (key->isEqualTo(batteryPrompt)) {
                OSNumber * value;
                getPropertyNumber(batteryPrompt);
                batnum = value->unsigned8BitValue();
                updateBattery(value->unsigned8BitValue());
            } else if (key->isEqualTo("setCMstart")) {
                OSNumber * value;
                getPropertyNumber("setCMstart");
                setConservation(setCMstart, value->unsigned8BitValue());
            } else if (key->isEqualTo("setCMstop")) {
                OSNumber * value;
                getPropertyNumber("setCMstop");
                setConservation(setCMstop, value->unsigned8BitValue());
            } else if (key->isEqualTo("setCMInhibitCharge")) {
                OSNumber * value;
                getPropertyNumber("setCMInhibitCharge");
                setConservation(setCMInhibitCharge, value->unsigned8BitValue());
            } else if (key->isEqualTo("setCMForceDischarge")) {
                OSNumber * value;
                getPropertyNumber("setCMForceDischarge");
                setConservation(setCMForceDischarge, value->unsigned8BitValue());
            } else if (key->isEqualTo("setCMPeakShiftState")) {
                OSNumber * value;
                getPropertyNumber("setCMPeakShiftState");
                setConservation(setCMPeakShiftState, value->unsigned8BitValue());
            } else if (key->isEqualTo(mutePrompt)) {
                OSNumber * value;
                getPropertyNumber(mutePrompt);
                setMutestatus(value->unsigned32BitValue());
            } else if (key->isEqualTo(muteLEDPrompt)) {
                OSBoolean * value;
                getPropertyBoolean(muteLEDPrompt);
                setMuteLEDStatus(value->getValue());
            } else if (key->isEqualTo(micMuteLEDPrompt)) {
                OSNumber * value;
                getPropertyNumber(micMuteLEDPrompt);
                setMicMuteLEDStatus(value->unsigned32BitValue());
            } else if (key->isEqualTo(muteSupportPrompt)) {
                OSBoolean * value;
                getPropertyBoolean(muteSupportPrompt);
                setMuteSupport(value->getValue());
            } else if (key->isEqualTo(beepPrompt)) {
                OSBoolean * status = OSDynamicCast(OSBoolean, dict->getObject(beepPrompt));
                if (status != nullptr) {
                    enableBeep(status->getValue());
                    continue;
                }
                OSNumber * value;
                getPropertyNumber(beepPrompt);
                setBeepStatus(value->unsigned8BitValue());
            } else if (key->isEqualTo(SSTPrompt)) {
                OSNumber * value;
                getPropertyNumber(SSTPrompt);
                setSSTStatus(value->unsigned32BitValue());
            } else if (key->isEqualTo(LEDPrompt)) {
                OSNumber * value;
                getPropertyNumber(LEDPrompt);
                if (value->unsigned32BitValue() > 0xff) {
                    AlwaysLog(valueInvalid, LEDPrompt);
                    continue;
                } else {
                    setLEDStatus(value->unsigned8BitValue());
                }
            } else if (key->isEqualTo(fanControlPrompt)) {
                OSNumber * value;
                getPropertyNumber(fanControlPrompt);
//                setFanControl(value->unsigned32BitValue());
                updateFanControl(value->unsigned32BitValue());
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

bool ThinkVPC::updateFanControl(UInt32 status, bool update) {
    OSObject* params[] = {
        OSNumber::withNumber(status, 32)
    };

    if (vpc->evaluateInteger(getThermalControl, &thermalstate, params, 1) != kIOReturnSuccess) {
        AlwaysLog(updateFailure, __func__);
        return false;
    }

    if (update)
        DebugLog(updateSuccess, __func__, thermalstate);

    setProperty(fanControlPrompt, thermalstate, 32);
    return true;
}

bool ThinkVPC::setFanControl(int level) {
    UInt32 result;

    OSObject* params[] = {
        OSNumber::withNumber((level ? BIT(18) : 0), 32)
    };

    if (vpc->evaluateInteger(setThermalControl, &result, params, 1) != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, fanControlPrompt);
        return false;
    }
    if (!result) {
        AlwaysLog(toggleFailure, "FanControl0");
        return false;
    }

    DebugLog(toggleSuccess, fanControlPrompt, (level ? BIT(18) : 0), (level ? "7" : "auto"));
    setProperty(fanControlPrompt, level, 32);
    return true;
}

bool ThinkVPC::updateMuteLEDStatus(bool update) {
    OSObject* params[] = {
        OSNumber::withNumber(0ULL, 32)
    };

    if (vpc->evaluateInteger(getAudioMuteLED, &muteLEDstate, params, 1) != kIOReturnSuccess) {
        AlwaysLog(updateFailure, __func__);
        return false;
    }

    if (update)
        DebugLog(updateSuccess, __func__, muteLEDstate);

    setProperty(muteLEDPrompt, muteLEDstate, 32);
    return true;
}

bool ThinkVPC::setMuteSupport(bool support) {
    UInt32 result;

    OSObject* params[] = {
        OSNumber::withNumber(support, 32)
    };

    if (vpc->evaluateInteger(setAudioMuteSupport, &result, params, 1) != kIOReturnSuccess || result & TPACPI_AML_MUTE_ERROR_STATE_MASK) {
        AlwaysLog(toggleFailure, muteSupportPrompt);
        return false;
    }

    DebugLog(toggleSuccess, muteSupportPrompt, support, (support ? "disable" : "enable"));
    setProperty(muteSupportPrompt, support);
    return true;
}

bool ThinkVPC::setBeepStatus(UInt8 status) {
    OSObject* params[] = {
        OSNumber::withNumber(status, 32)
    };

    if (ec->evaluateObject(setBeep, nullptr, params, 1) != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, beepPrompt);
        return false;
    }

    DebugLog(toggleSuccess, beepPrompt, status, (status ? "on" : "off"));
    setProperty(beepPrompt, status);
    return true;
}

bool ThinkVPC::enableBeep(bool enable) {
    OSObject* params[] = {
        OSNumber::withNumber(enable, 32)
    };

    if (vpc->evaluateObject(setHKEYBeep, nullptr, params, 1) != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, beepPrompt);
        return false;
    }

    DebugLog(toggleSuccess, beepPrompt, enable, (enable ? "enabled" : "disabled"));
    setProperty(beepPrompt, enable);
    return true;
}

bool ThinkVPC::setLEDStatus(UInt8 status) {
    OSObject* params[] = {
        OSNumber::withNumber(status & 0x0f, 32),
        OSNumber::withNumber(status & 0xf0, 32),
    };

    if (ec->evaluateObject(setLED, nullptr, params, 2) != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, LEDPrompt);
        return false;
    }

    DebugLog(toggleSuccess, LEDPrompt, status, (status ? "on" : "off"));
    setProperty(LEDPrompt, status);
    return true;
}

bool ThinkVPC::setMuteLEDStatus(bool status) {
    UInt32 result;

    OSObject* params[] = {
        OSNumber::withNumber(status, 32)
    };

    if (vpc->evaluateInteger(setAudioMuteLED, &result, params, 1) != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, muteLEDPrompt);
        return false;
    }

    DebugLog(toggleSuccess, muteLEDPrompt, status, (status ? "on" : "off"));
    setProperty(muteLEDPrompt, status);
    return true;
}

bool ThinkVPC::updateMicMuteLEDStatus(bool update) {
    if (vpc->evaluateInteger(getMicMuteLED, &micMuteLEDcap) != kIOReturnSuccess) {
        AlwaysLog(updateFailure, __func__);
        return false;
    }

    if (update)
        DebugLog(updateSuccess, __func__, micMuteLEDcap);

    if (micMuteLEDcap & TPACPI_AML_MIC_MUTE_HDMC)
        setProperty(micMuteLEDPrompt, "HDMC");
    else
        setProperty(micMuteLEDPrompt, micMuteLEDcap, 32);
    return true;
}

bool ThinkVPC::setMicMuteLEDStatus(UInt32 status) {
    UInt32 result;

    OSObject* params[] = {
        OSNumber::withNumber(status, 32)
    };

    if (vpc->evaluateInteger(setMicMuteLED, &result, params, 1) != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, micMuteLEDPrompt);
        return false;
    }

    micMuteLEDstate = status;
    DebugLog(toggleSuccess, micMuteLEDPrompt, status, (status ? "on / blink" : "off"));
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
                AlwaysLog("message: Unknown ACPI notification\n");
            else if (*((UInt32 *) argument) == kIOACPIMessageReserved)
                updateVPC();
            else
                AlwaysLog("message: Unknown ACPI notification 0x%04x\n", *((UInt32 *) argument));
            break;

        default:
            if (argument)
                AlwaysLog("message: type=%x, provider=%s, argument=0x%04x\n", type, provider->getName(), *((UInt32 *) argument));
            else
                AlwaysLog("message: type=%x, provider=%s\n", type, provider->getName());
    }

    return kIOReturnSuccess;
}

void ThinkVPC::updateVPC() {
    UInt32 result;
    if (vpc->evaluateInteger(getHKEYevent, &result) != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, HotKeyPrompt);
        return;
    }

    if (!result) {
        AlwaysLog("empty HKEY event");
        return;
    }

    switch (result >> 0xC) {
        case 1:
            switch (result) {
                case TP_HKEY_EV_KBD_LIGHT:
                    updateBacklight();

                case TP_HKEY_EV_BRGHT_UP:
                case TP_HKEY_EV_BRGHT_DOWN:
                    break;

                case TP_HKEY_EV_MIC_MUTE:
                    setMicMuteLEDStatus(micMuteLEDstate ? 0 : 2);
                    break;
                    
                default:
                    AlwaysLog("Hotkey(MHKP) key presses event: 0x%x \n", result);
                    break;
            }
            break;

        case 2:
            AlwaysLog("Hotkey(MHKP) wakeup reason event: 0x%x \n", result);
            break;

        case 3:
            AlwaysLog("Hotkey(MHKP) bay-related wakeup event: 0x%x \n", result);
            break;

        case 4:
            AlwaysLog("Hotkey(MHKP) dock-related event: 0x%x \n", result);
            break;

        case 5:
            switch (result) {
                case TP_HKEY_EV_PEN_INSERTED:
                    AlwaysLog("tablet pen inserted into bay\n");
                    break;

                case TP_HKEY_EV_PEN_REMOVED:
                    AlwaysLog("tablet pen removed from bay\n");
                    break;

                case TP_HKEY_EV_TABLET_TABLET:
                    AlwaysLog("tablet mode\n");
                    break;

                case TP_HKEY_EV_TABLET_NOTEBOOK:
                    AlwaysLog("normal mode\n");
                    break;

                case TP_HKEY_EV_LID_CLOSE:
                    AlwaysLog("Lid closed\n");
                    break;

                case TP_HKEY_EV_LID_OPEN:
                    AlwaysLog("Lid opened\n");
                    break;

                case TP_HKEY_EV_BRGHT_CHANGED:
                    AlwaysLog("brightness changed\n");
                    break;

                default:
                    AlwaysLog("Hotkey(MHKP) unknown human interface event: 0x%x \n", result);
                    break;
            }
            break;

        case 6:
            switch (result) {
                case TP_HKEY_EV_THM_TABLE_CHANGED:
                    AlwaysLog("Thermal Table has changed\n");
                    break;

                case TP_HKEY_EV_THM_CSM_COMPLETED:
                    AlwaysLog("Thermal Control Command set completed (DYTC)\n");
                    UInt64 output;
                    if (DYTCCap && DYTCCommand(DYTC_CMD_GET, &output)) {
                        DYTCMode = output;
                        setProperty("DYTCMode", output, 64);
                        setProperty("lapmode", output & BIT(DYTC_GET_LAPMODE_BIT));
                        AlwaysLog("DYTC lapmode 0x%llx %s\n", output, (output & BIT(DYTC_GET_LAPMODE_BIT) ? "on" : "off"));
                    }
                    break;

                case TP_HKEY_EV_THM_TRANSFM_CHANGED:
                    AlwaysLog("Thermal Transformation changed (GMTS)\n");
                    break;

                case TP_HKEY_EV_ALARM_BAT_HOT:
                    AlwaysLog("THERMAL ALARM: battery is too hot!\n");
                    break;

                case TP_HKEY_EV_ALARM_BAT_XHOT:
                    AlwaysLog("THERMAL EMERGENCY: battery is extremely hot!\n");
                    break;

                case TP_HKEY_EV_ALARM_SENSOR_HOT:
                    AlwaysLog("THERMAL ALARM: a sensor reports something is too hot!\n");
                    break;

                case TP_HKEY_EV_ALARM_SENSOR_XHOT:
                    AlwaysLog("THERMAL EMERGENCY: a sensor reports something is extremely hot!\n");
                    break;

                case TP_HKEY_EV_AC_CHANGED:
                    AlwaysLog("AC status changed\n");
                    break;

                case TP_HKEY_EV_KEY_NUMLOCK:
                    AlwaysLog("Numlock\n");
                    break;

                case TP_HKEY_EV_KEY_FN:
                    AlwaysLog("Fn\n");
                    break;

                case TP_HKEY_EV_KEY_FN_ESC:
                    AlwaysLog("Fn+Esc\n");
                    break;

                case TP_HKEY_EV_LID_STATUS_CHANGED:
                    break;

                case TP_HKEY_EV_TABLET_CHANGED:
                    AlwaysLog("tablet mode has changed\n");
                    break;

                case TP_HKEY_EV_PALM_DETECTED:
                    AlwaysLog("palm detected hovering the keyboard\n");
                    break;

                case TP_HKEY_EV_PALM_UNDETECTED:
                    AlwaysLog("palm undetected hovering the keyboard\n");
                    break;

                default:
                    AlwaysLog("Hotkey(MHKP) unknown thermal alarms/notices and keyboard event: 0x%x \n", result);
                    break;
            }
            break;

        case 7:
            AlwaysLog("Hotkey(MHKP) misc event: 0x%x \n", result);
            break;

        default:
            AlwaysLog("Hotkey(MHKP) unknown event: 0x%x \n", result);
            break;
    }
}

bool ThinkVPC::setHotkeyStatus(bool enable) {
    UInt32 result;

    OSObject* params[] = {
        OSNumber::withNumber(enable, 32)
    };

    if (vpc->evaluateInteger(setHKEYstatus, &result, params, 1) != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, HotKeyPrompt);
        return false;
    }

    DebugLog(toggleSuccess, HotKeyPrompt, enable, (enable ? "enabled" : "disabled"));
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
        AlwaysLog(updateFailure, KeyboardPrompt);
        return false;
    }

    backlightLevel = state & 0x3;

    if (update) {
        DebugLog(updateSuccess, KeyboardPrompt, state);
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
        AlwaysLog(toggleFailure, backlightPrompt);
        return false;
    }

    backlightLevel = level;
    DebugLog(toggleSuccess, backlightPrompt, backlightLevel, (backlightLevel ? "on" : "off"));
    setProperty(backlightPrompt, backlightLevel, 32);

    return true;
}

bool ThinkVPC::setSSTStatus(UInt32 value) {
    OSObject* params[] = {
        OSNumber::withNumber(value, 32)
    };
    
    if (vpc->evaluateObject("CSSI", nullptr, params, 1) != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, SSTPrompt);
        return false;
    }

    char const *property[5] = {"Indicator off", "Working", "Waking", "Sleeping", "Sleeping (S4)"};
    
    DebugLog(toggleSuccess, SSTPrompt, value, property[value]);
    return true;
}

IOReturn ThinkVPC::setPowerState(unsigned long powerState, IOService *whatDevice){
    if (super::setPowerState(powerState, whatDevice) != kIOPMAckImplied)
        return kIOReturnInvalid;

    if (powerState == 0) {
        muteLEDstateSaved = muteLEDstate;
        if (muteLEDstate)
            setBacklight(0);
        setSSTStatus(3);
    } else {
        if (muteLEDstateSaved)
            setBacklight(muteLEDstateSaved);
        setSSTStatus(0);
    }

    return kIOPMAckImplied;
}

