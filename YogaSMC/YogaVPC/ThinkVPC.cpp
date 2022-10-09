//  SPDX-License-Identifier: GPL-2.0-only
//
//  ThinkVPC.cpp
//  YogaSMC
//
//  Created by Zhen on 7/12/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "ThinkEvents.h"
#include "ThinkVPC.hpp"

OSDefineMetaClassAndStructors(ThinkVPC, YogaVPC);

void ThinkVPC::updateAll() {
    if (hotkey_legacy) {
        OSObject* result;
        if (vpc->evaluateObject(getHKEYmask, &result) != kIOReturnSuccess)
            AlwaysLog(toggleFailure, "LegacyNotificationMask");
        else
            setProperty("HKEY Setting", result);
        OSSafeReleaseNULL(result);
    } else {
        OSDictionary *KBDPresent = OSDictionary::withCapacity(3);
        getNotificationMask(1, KBDPresent);
        getNotificationMask(2, KBDPresent);
        getNotificationMask(3, KBDPresent);
        setProperty("HKEY Setting", KBDPresent);
        KBDPresent->release();
    }
    updateBattery(BAT_ANY);
    updateFnLock();
    updateMutestatus();
    updateMuteLEDStatus();
    updateMicMuteLEDStatus();
    updateKBDLocale();
    updateVPC();
    updateYogaMode();
    super::updateAll();
}

bool ThinkVPC::updateConservation(const char * method, OSDictionary *bat, bool update) {
    UInt32 result;
    if (evaluateIntegerParam(method, &result, batnum) != kIOReturnSuccess) {
        AlwaysLog(updateFailure, method);
        return false;
    }

    if (update)
        DebugLog(updateSuccess, method, result);

    OSNumber *value;
    setPropertyNumber(bat, method, result, 32);

    return true;
}

bool ThinkVPC::setConservation(const char * method, UInt8 value) {
    UInt32 result;
    if (evaluateIntegerParam(method, &result, (batnum << 8) + value) != kIOReturnSuccess) {
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
    if (evaluateIntegerParam(setAudioMutestatus, &result, value) != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, __func__);
        return false;
    }

    DebugLog(toggleSuccess, __func__, mutestate, (mutestate == 2 ? "SW" : "HW"));
    setProperty(mutePrompt, mutestate, 32);
    return true;
}

void ThinkVPC::getNotificationMask(UInt32 index, OSDictionary *KBDPresent) {
    UInt32 result;
    if (evaluateIntegerParam(getHKEYmask, &result, index) != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, __func__);
        return;
    }

    OSNumber *value;
    switch (index) {
        case 1:
            setPropertyNumber(KBDPresent, "DHKN", result, 32);
            break;

        case 2:
            setPropertyNumber(KBDPresent, "DHKE", result, 32);
            break;

        case 3:
            setPropertyNumber(KBDPresent, "DHKF", result, 32);
            break;

        default:
            break;
    }
}

bool ThinkVPC::setNotificationMask(UInt32 i, UInt32 all_mask, UInt32 offset, bool enable) {
    OSObject* params[] = {
        OSNumber::withNumber(i + offset + 1, 32),
        OSNumber::withNumber(enable, 32),
    };

    IOReturn ret = vpc->evaluateObject(setHKEYmask, nullptr, params, 2);
    params[0]->release();
    params[1]->release();

    if (ret != kIOReturnSuccess) {
        AlwaysLog("set HKEY mask BIT %x failed", i);
        return false;
    }
    DebugLog("set HKEY mask BIT %x", i);
    return true;
}

bool ThinkVPC::initVPC() {
    super::initVPC();
    UInt32 version;

    if (vpc->evaluateInteger(getHKEYversion, &version) != kIOReturnSuccess)
    {
        AlwaysLog(initFailure, getHKEYversion);
        return false;
    }

    OSObject *value;
    KBDProperty = OSDictionary::withCapacity(1);
    DebugLog("HKEY interface version %x", version);
    setPropertyNumber(KBDProperty, "Version", version, 32);
    switch (version >> 8) {
        case 1:
            hotkey_legacy = true;
            updateAdaptiveKBD(0);
            break;

        case 2:
            updateAdaptiveKBD(1);
            updateAdaptiveKBD(2);
            updateAdaptiveKBD(3);
            break;

        default:
            AlwaysLog("Unknown HKEY version");
            break;
    }

    if (hotkey_all_mask) {
        for (UInt32 i = 0; i < 0x20; i++) {
            if ((hotkey_all_mask & BIT(i)) &&
                !setNotificationMask(i, hotkey_all_mask, DHKN_MASK_offset))
                break;
        }
    } else {
        AlwaysLog("Failed to acquire hotkey_all_mask");
    }
    setProperty("HKEY Property", KBDProperty);
    KBDProperty->release();

    setHotkeyStatus(true);

    UInt32 state;
    if (evaluateIntegerParam(getKBDBacklightLevel, &state, 0) == kIOReturnSuccess)
        backlightCap = BIT(KBD_BACKLIGHT_CAP_BIT) & state;

    if (!backlightCap)
        setProperty(backlightPrompt, "unsupported");

    setProperty(autoBacklightPrompt, automaticBacklightMode, 8);

    LEDSupport = (ec->validateObject(setLED) == kIOReturnSuccess);
    setProperty("LEDSupport", LEDSupport);

    yogaModeSupport = (vpc->validateObject(getMultiModeState) == kIOReturnSuccess);

    setMicMuteLEDStatus(0);
    return true;
}

bool ThinkVPC::exitVPC() {
    setHotkeyStatus(false);
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
        if (evaluateIntegerParam(getHKEYAdaptive, &result, arg) != kIOReturnSuccess) {
            AlwaysLog(updateFailure, __func__);
            return false;
        }
    }

    AlwaysLog("%s %d %x", __func__, arg, result);
    OSObject *value;
    switch (arg) {
        case 0:
        case 1:
            hotkey_all_mask = result;
            setPropertyNumber(KBDProperty, "hotkey_all_mask", result, 32);
            break;

        case 2:
            if (result) {
                hotkey_adaptive_all_mask = result;
                setPropertyNumber(KBDProperty, "hotkey_adaptive_all_mask", result, 32);
            }
            break;

        case 3:
            hotkey_alter_all_mask = result;
            setPropertyNumber(KBDProperty, "hotkey_alter_all_mask", result, 32);
            break;

        default:
            AlwaysLog("Unknown MHKA command");
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

    OSDictionary *bat = OSDictionary::withCapacity(5);
    if (updateConservation(getCMstart, bat) &&
        updateConservation(getCMstop, bat) &&
        updateConservation(getCMInhibitCharge, bat) &&
        updateConservation(getCMForceDischarge, bat) &&
        updateConservation(getCMPeakShiftState, bat))
        DebugLog(updateSuccess, batteryPrompt, batnum);
    else
        AlwaysLog(updateFailure, batteryPrompt);

    switch (batnum) {
        case BAT_ANY:
            setProperty("BAT_ANY", bat);
            break;

        case BAT_PRIMARY:
            setProperty("BAT_PRIMARY", bat);
            break;

        case BAT_SECONDARY:
            setProperty("BAT_SECONDARY", bat);
            break;
    }
    bat->release();
}

void ThinkVPC::setPropertiesGated(OSObject *props) {
    OSDictionary *dict = OSDynamicCast(OSDictionary, props);
    if (!dict)
        return;

    //    AlwaysLog("%d objects in properties", dict->getCount());
    OSCollectionIterator* i = OSCollectionIterator::withCollection(dict);
    IOReturn ret;

    if (i) {
        while (OSString* key = OSDynamicCast(OSString, i->getNextObject())) {
            if (key->isEqualTo(batteryPrompt)) {
                OSNumber *value;
                getPropertyNumber(batteryPrompt);
                batnum = value->unsigned8BitValue();
                updateBattery(value->unsigned8BitValue());
            } else if (key->isEqualTo("setCMstart")) {
                OSNumber *value;
                getPropertyNumber("setCMstart");
                setConservation(setCMstart, value->unsigned8BitValue());
            } else if (key->isEqualTo("setCMstop")) {
                OSNumber *value;
                getPropertyNumber("setCMstop");
                setConservation(setCMstop, value->unsigned8BitValue());
            } else if (key->isEqualTo("setCMInhibitCharge")) {
                OSNumber *value;
                getPropertyNumber("setCMInhibitCharge");
                setConservation(setCMInhibitCharge, value->unsigned8BitValue());
            } else if (key->isEqualTo("setCMForceDischarge")) {
                OSNumber *value;
                getPropertyNumber("setCMForceDischarge");
                setConservation(setCMForceDischarge, value->unsigned8BitValue());
            } else if (key->isEqualTo("setCMPeakShiftState")) {
                OSNumber *value;
                getPropertyNumber("setCMPeakShiftState");
                setConservation(setCMPeakShiftState, value->unsigned8BitValue());
            } else if (key->isEqualTo("GHSL")) {
                UInt32 result;
                if (evaluateIntegerParam("GHKL", &result, 0) == kIOReturnSuccess)
                    DebugLog(updateSuccess, "GHSL", result);
                else
                    AlwaysLog(updateFailure, "GHSL");
#ifdef DEBUG
            } else if (key->isEqualTo(localePrompt)) {
                OSNumber *value;
                getPropertyNumber(localePrompt);
                setKBDLocale(value->unsigned16BitValue());
            } else if (key->isEqualTo("SCPF")) {
                OSNumber *value;
                getPropertyNumber("SCPF");
                UInt32 raw = value->unsigned32BitValue();
                OSObject* params[4] = {
                    OSNumber::withNumber((raw >> 24) & 0xff, 8),
                    OSNumber::withNumber((raw >> 16) & 0xff, 8),
                    OSNumber::withNumber((raw >> 8) & 0xff, 8),
                    OSNumber::withNumber((raw & 0xff), 8)
                };

                ret = vpc->evaluateObject("SCPF", nullptr, params, 4);
                if (ret == kIOReturnSuccess)
                    DebugLog(updateSuccess, "SCPF", value->unsigned8BitValue());
                else
                    AlwaysLog("%s evaluation failed 0x%x", "SCPF", ret);
                params[0]->release();
                params[1]->release();
                params[2]->release();
                params[3]->release();
#endif
            } else if (key->isEqualTo(mutePrompt)) {
                OSNumber *value;
                getPropertyNumber(mutePrompt);
                setMutestatus(value->unsigned32BitValue());
            } else if (key->isEqualTo(muteLEDPrompt)) {
                OSBoolean *value;
                getPropertyBoolean(muteLEDPrompt);
                setMuteLEDStatus(value->getValue());
            } else if (key->isEqualTo(micMuteLEDPrompt)) {
                OSNumber *value;
                getPropertyNumber(micMuteLEDPrompt);
                setMicMuteLEDStatus(value->unsigned32BitValue());
            } else if (key->isEqualTo(muteSupportPrompt)) {
                OSBoolean *value;
                getPropertyBoolean(muteSupportPrompt);
                setMuteSupport(value->getValue());
            } else if (key->isEqualTo(beepPrompt)) {
                OSBoolean * status = OSDynamicCast(OSBoolean, dict->getObject(beepPrompt));
                if (status != nullptr) {
                    enableBeep(status->getValue());
                    continue;
                }
                OSNumber *value;
                getPropertyNumber(beepPrompt);
                setBeepStatus(value->unsigned8BitValue());
            } else if (key->isEqualTo(SSTPrompt)) {
                OSNumber *value;
                getPropertyNumber(SSTPrompt);
                if (value->unsigned32BitValue() >= 5)
                    AlwaysLog(valueInvalid, SSTPrompt);
                else
                    setSSTStatus(value->unsigned32BitValue());
            } else if (key->isEqualTo(LEDPrompt)) {
                if (!LEDSupport) {
                    AlwaysLog(notSupported, LEDPrompt);
                    continue;
                }
                OSNumber *value;
                getPropertyNumber(LEDPrompt);
                if (value->unsigned32BitValue() >= 0xff)
                    AlwaysLog(valueInvalid, LEDPrompt);
                else
                    setLEDStatus(value->unsigned8BitValue());
            } else if (key->isEqualTo(fanControlPrompt)) {
                OSNumber *value;
                getPropertyNumber(fanControlPrompt);
//                setFanControl(value->unsigned32BitValue());
                updateFanControl(value->unsigned32BitValue());
            } else if (key->isEqualTo(fanSpeedPrompt)) {
                OSNumber *value;
                getPropertyNumber(fanSpeedPrompt);
                UInt8 speed = value->unsigned8BitValue();
                if (speed > 7) {
                    speed = speed < 0x80 ? 0x47 : 0x84;
                }
                ret = method_we1b(0x2f, speed);
                if (ret == kIOReturnSuccess)
                    DebugLog(updateSuccess, fanSpeedPrompt, speed);
                else
                    AlwaysLog("%s update failed 0x%x", fanSpeedPrompt, ret);
            } else if (key->isEqualTo(updatePrompt)) {
                updateAll();
            } else {
                OSDictionary *entry = OSDictionary::withCapacity(1);
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
    if (evaluateIntegerParam(getThermalControl, &thermalstate, status) != kIOReturnSuccess) {
        AlwaysLog(updateFailure, __func__);
        return false;
    }

    if (update) {
        DebugLog(updateSuccess, __func__, thermalstate);
    }
    setProperty(fanControlPrompt, thermalstate, 32);
    return true;
}

bool ThinkVPC::setFanControl(int level) {
    UInt32 result;
    if (evaluateIntegerParam(setThermalControl, &result, (level ? BIT(18) : 0)) != kIOReturnSuccess) {
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
    if (evaluateIntegerParam(getAudioMuteLED, &muteLEDstate, 0) != kIOReturnSuccess) {
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
    IOReturn ret;

    ret = evaluateIntegerParam(setAudioMuteSupport, &result, support);
    if (ret != kIOReturnSuccess || result & TPACPI_AML_MUTE_ERROR_STATE_MASK) {
        AlwaysLog(toggleError, muteSupportPrompt, ret ?: result);
        return false;
    }

    DebugLog(toggleSuccess, muteSupportPrompt, support, (support ? "disable" : "enable"));
    setProperty(muteSupportPrompt, support);
    return true;
}

bool ThinkVPC::setBeepStatus(UInt8 status) {
    if (evaluateIntegerParam(setBeep, nullptr, status) != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, beepPrompt);
        return false;
    }

    DebugLog(toggleSuccess, beepPrompt, status, (status ? "on" : "off"));
    setProperty(beepPrompt, status);
    return true;
}

bool ThinkVPC::enableBeep(bool enable) {
    if (evaluateIntegerParam(setHKEYBeep, nullptr, enable) != kIOReturnSuccess) {
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

    IOReturn ret = ec->evaluateObject(setLED, nullptr, params, 2);
    params[0]->release();
    params[1]->release();
    if (ret != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, LEDPrompt);
        return false;
    }

    DebugLog(toggleSuccess, LEDPrompt, status, (status ? "on" : "off"));
    return true;
}

bool ThinkVPC::setMuteLEDStatus(bool status) {
    UInt32 result;
    if (evaluateIntegerParam(setAudioMuteLED, &result, status) != kIOReturnSuccess) {
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
    if (evaluateIntegerParam(setMicMuteLED, nullptr, status) != kIOReturnSuccess) {
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
        // TODO: determine default value for charge level
        case kSMC_getConservation:
        case kSMC_setConservation:
            break;

        case kIOACPIMessageDeviceNotification:
            if (argument && *(reinterpret_cast<UInt32 *>(argument)) == kIOACPIMessageReserved) {
                updateVPC();
                break;
            }

        default:
            return super::message(type, provider, argument);
    }

    return kIOReturnSuccess;
}

UInt32 ThinkVPC::updateThermalMode() {
    UInt32 mode;
    if (evaluateIntegerParam(getThermalModeState, &mode, 0) != kIOReturnSuccess)
        mode = 0;
    return mode;
}

UInt32 ThinkVPC::updateYogaMode() {
    if (!yogaModeSupport)
        return 0;

    UInt32 mode;
    if (evaluateIntegerParam(getMultiModeState, &mode, 0) != kIOReturnSuccess) {
        AlwaysLog("failed to evaluate tablet mode");
        return 0;
    }

#ifdef DEBUG
    UInt32 rawMode;
    if (ec->evaluateInteger("CMMD", &rawMode) != kIOReturnSuccess)
        AlwaysLog("failed to evaluate raw tablet mode");
    else
        DebugLog("tablet mode: %x, raw: %x", mode, rawMode);
#else
    DebugLog("tablet mode: %x", mode);
#endif
    if (!validModes) {
        UInt16 type = mode >> 16;
        setProperty("MultiModeType", type, 16);
        switch (type) {
            case 1:
                validModes = TP_ACPI_MULTI_MODE_LAPTOP |
                             TP_ACPI_MULTI_MODE_TABLET |
                             TP_ACPI_MULTI_MODE_STAND_TENT;
                break;
            case 2:
                validModes = TP_ACPI_MULTI_MODE_LAPTOP |
                             TP_ACPI_MULTI_MODE_FLAT |
                             TP_ACPI_MULTI_MODE_TABLET |
                             TP_ACPI_MULTI_MODE_STAND |
                             TP_ACPI_MULTI_MODE_TENT;
                break;
            case 3:
                validModes = TP_ACPI_MULTI_MODE_LAPTOP |
                             TP_ACPI_MULTI_MODE_FLAT;
                break;
            case 4:
            case 5:
                /* In mode 4, FLAT is not specified as a valid mode. However,
                 * it can be seen at least on the X1 Yoga 2nd Generation.
                 */
                validModes = TP_ACPI_MULTI_MODE_LAPTOP |
                             TP_ACPI_MULTI_MODE_FLAT |
                             TP_ACPI_MULTI_MODE_TABLET |
                             TP_ACPI_MULTI_MODE_STAND |
                             TP_ACPI_MULTI_MODE_TENT;
                break;
                
            default:
                AlwaysLog("unknown tablet mode: %x", mode);
                validModes = TP_ACPI_MULTI_MODE_UNKNOWN;
                break;
        }
    }
    UInt32 data = mode & 0xf;
    switch (data) {
        case 1:
//            mode = TP_ACPI_MULTI_MODE_LAPTOP;
            if (validModes & TP_ACPI_MULTI_MODE_TABLET_LIKE)
                setTopCase(true);
            setProperty("YogaMode", "Laptop");
            break;
        case 2:
//            mode = TP_ACPI_MULTI_MODE_FLAT;
            if (validModes & TP_ACPI_MULTI_MODE_TABLET_LIKE)
                setTopCase(true);
            setProperty("YogaMode", "Flat");
            break;
        case 3:
//            mode = TP_ACPI_MULTI_MODE_TABLET;
            setTopCase(clamshellMode);
            setProperty("YogaMode", "Tablet");
            break;
        case 4:
//            if ((mode >> 16) == 1)
//                mode = TP_ACPI_MULTI_MODE_STAND_TENT;
//            else
//                mode = TP_ACPI_MULTI_MODE_STAND;
            setTopCase(clamshellMode);
            if ((mode >> 16) == 1)
                setProperty("YogaMode", "Stand/Tent");
            else
                setProperty("YogaMode", "Stand");
            break;
        case 5:
//            mode = TP_ACPI_MULTI_MODE_TENT;
            setTopCase(clamshellMode);
            setProperty("YogaMode", "Tent");
            break;
        default:
//            if (type == 5 && value == 0xffff)
//                DebugLog("Multi mode status is undetected, assuming laptop");
            setTopCase(true);
            AlwaysLog("unknown tablet mode: %x", mode);
            break;
    }
    return data;
}

void ThinkVPC::updateVPC(UInt32 event) {
    UInt32 result;
    if (vpc->evaluateInteger(getHKEYevent, &result) != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, HotKeyPrompt);
        return;
    }

    if (!result) {
        DebugLog("empty HKEY event");
        return;
    }

    UInt32 data = 0;
    UInt64 time = 0;
    switch (result >> 0xC) {
        case 1:
            switch (result) {
                case TP_HKEY_EV_KBD_LIGHT:
                    updateBacklight(true);
                    data = backlightLevel;
                    if (isActive(&backlightStimulation))
                        data |= 0x10;
                    break;

                case TP_HKEY_EV_SLEEP:
                    if (!client)
                        vpc->evaluateObject(setHKEYsleep);
                    break;

                case TP_HKEY_EV_MIC_MUTE:
                    if (!client)
                        setMicMuteLEDStatus(micMuteLEDstate ? 0 : 2);
                    break;

                case TP_HKEY_EV_KEYBOARD:
                    toggleKeyboard();
                    data = Keyboardenabled;
                    break;

                default:
                    DebugLog("Hotkey(MHKP) key presses event: 0x%x", result);
                    break;
            }
            time = 1;
            break;

        case 2:
            DebugLog("Hotkey(MHKP) wakeup reason event: 0x%x", result);
            break;

        case 3:
            DebugLog("Hotkey(MHKP) bay-related wakeup event: 0x%x", result);
            break;

        case 4:
            DebugLog("Hotkey(MHKP) dock-related event: 0x%x", result);
            break;

        case 5:
            switch (result) {
                case TP_HKEY_EV_PEN_INSERTED:
                    DebugLog("tablet pen inserted into bay");
                    break;

                case TP_HKEY_EV_PEN_REMOVED:
                    DebugLog("tablet pen removed from bay");
                    break;

                case TP_HKEY_EV_TABLET_TABLET:
                    DebugLog("tablet mode");
                    break;

                case TP_HKEY_EV_TABLET_NOTEBOOK:
                    DebugLog("normal mode");
                    break;

                case TP_HKEY_EV_LID_CLOSE:
                    DebugLog("Lid closed");
                    break;

                case TP_HKEY_EV_LID_OPEN:
                    DebugLog("Lid opened");
                    break;

                case TP_HKEY_EV_BRGHT_CHANGED:
                    DebugLog("brightness changed");
                    break;

                default:
                    DebugLog("Hotkey(MHKP) unknown human interface event: 0x%x", result);
                    break;
            }
            break;

        case 6:
            switch (result) {
                case TP_HKEY_EV_THM_TABLE_CHANGED:
                    DebugLog("Thermal Table has changed");
                    break;

                case TP_HKEY_EV_THM_CSM_COMPLETED:
                    DebugLog("Thermal Control Command set completed (DYTC)");
                    updateDYTC();
                    break;

                case TP_HKEY_EV_THM_TRANSFM_CHANGED:
                    data = updateThermalMode();
                    if (data & 0x00010000)
                        DebugLog("Thermal Transformation changed: 0x%x", data);
                    else
                        AlwaysLog("Thermal Transformation changed: failed to evaluate GMTS");
                    break;

                case TP_HKEY_EV_ALARM_BAT_HOT:
                    AlwaysLog("THERMAL ALARM: battery is too hot!");
                    break;

                case TP_HKEY_EV_ALARM_BAT_XHOT:
                    AlwaysLog("THERMAL EMERGENCY: battery is extremely hot!");
                    break;

                case TP_HKEY_EV_ALARM_SENSOR_HOT:
                    AlwaysLog("THERMAL ALARM: a sensor reports something is too hot!");
                    break;

                case TP_HKEY_EV_ALARM_SENSOR_XHOT:
                    AlwaysLog("THERMAL EMERGENCY: a sensor reports something is extremely hot!");
                    break;

                case TP_HKEY_EV_AC_CHANGED:
                    DebugLog("AC status changed");
                    break;

                case TP_HKEY_EV_KEY_NUMLOCK:
                    DebugLog("Numlock");
                    time = 1;
                    break;

                case TP_HKEY_EV_KEY_FN:
                    DebugLog("Fn");
                    time = 1;
                    break;

                case TP_HKEY_EV_KEY_FN_ESC:
                    DebugLog("Fn+Esc");
                    time = 1;
                    updateFnLock();
                    data = FnlockMode;
                    break;

                case TP_HKEY_EV_LID_STATUS_CHANGED:
                    break;

                case TP_HKEY_EV_TABLET_CHANGED:
                    data = updateYogaMode();
                    if (clamshellMode) {
                        DebugLog("ClamshellMode enabled, skip Yoga mode notification");
                        return;
                    }
                    break;

                case TP_HKEY_EV_PALM_DETECTED:
                    DebugLog("palm detected hovering the keyboard");
                    break;

                case TP_HKEY_EV_PALM_UNDETECTED:
                    DebugLog("palm undetected hovering the keyboard");
                    break;

                default:
                    AlwaysLog("Hotkey(MHKP) unknown thermal alarms/notices and keyboard event: 0x%x", result);
                    break;
            }
            break;

        case 7:
            DebugLog("Hotkey(MHKP) misc event: 0x%x", result);
            break;

        default:
            DebugLog("Hotkey(MHKP) unknown event: 0x%x", result);
            break;
    }

    if (client)
        client->sendNotification(result, data);

    if (time != 0) {
        clock_get_uptime(&time);
        dispatchMessage(kPS2M_notifyKeyTime, &time);
    }
}

bool ThinkVPC::updateFnLock(bool update) {
    UInt32 result;
    if (vpc->evaluateInteger(getMediaKeyStatus, &result) != kIOReturnSuccess) {
        AlwaysLog(updateFailure, FnKeyPrompt);
        return false;
    }

    FnlockMode = result & 0x1;

    if (update) {
        DebugLog(toggleSuccess, FnKeyPrompt, result, (FnlockMode ? "enabled" : "disabled"));
    }
    setProperty(FnKeyPrompt, FnlockMode);
    return true;
}

bool ThinkVPC::setFnLock(bool enable) {
    UInt32 result;
    if (evaluateIntegerParam(setMediaKeyStatus, &result, enable) != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, FnKeyPrompt);
        return false;
    }

    FnlockMode = enable;
    DebugLog(toggleSuccess, FnKeyPrompt, result, (FnlockMode ? "enabled" : "disabled"));
    setProperty(FnKeyPrompt, FnlockMode);
    return true;
}

bool ThinkVPC::setHotkeyStatus(bool enable) {
    UInt32 result;
    if (evaluateIntegerParam(setHKEYstatus, &result, enable) != kIOReturnSuccess) {
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
    if (evaluateIntegerParam(getKBDBacklightLevel, &state, 0) != kIOReturnSuccess) {
        AlwaysLog(updateFailure, backlightPrompt);
        return false;
    }

    backlightLevel = state & 0x3;

    if (update) {
        DebugLog(updateSuccess, backlightPrompt, state);
    }
    setProperty(backlightPrompt, backlightLevel, 32);
    return true;
}

bool ThinkVPC::setBacklight(UInt32 level) {
    if (!backlightCap)
        return true;

    UInt32 result;
    IOReturn ret;

    ret = evaluateIntegerParam(setKBDBacklightLevel, &result, level);
    if (ret != kIOReturnSuccess || result != 0) {
        AlwaysLog(toggleError, backlightPrompt, ret ?: result);
        return false;
    }

    backlightLevel = level;
    DebugLog(toggleSuccess, backlightPrompt, backlightLevel, (backlightLevel ? "on" : "off"));
    setProperty(backlightPrompt, backlightLevel, 32);

    return true;
}

bool ThinkVPC::setSSTStatus(UInt32 value) {
#ifdef DEBUG
    char const *property[5] = {"Indicator off", "Working", "Waking", "Sleeping", "Hibernating"};
#endif
    if (vpc->validateObject("CSSI") == kIOReturnSuccess) {
        if (evaluateIntegerParam("CSSI", nullptr, value) != kIOReturnSuccess) {
            AlwaysLog(toggleFailure, "SST Proxy");
            return false;
        }

        DebugLog(toggleSuccess, "SST Proxy", value, property[value]);
        return true;
    }
    if (!LEDSupport)
        return true;
    // Replicate of _SI._SST
    // 0x0: power
    // 0x7: standby
    // 0xA: thinkdot
    bool status = true;
    switch (value) {
        case 0:
            status = setLEDStatus(0x00) && setLEDStatus(0x0A) && setLEDStatus(0x07);
            break;

        case 1:
//            if (SPS || WNTF)
//                setBeepStatus(0x05);
            status = setLEDStatus(0x80) && setLEDStatus(0x8A) && setLEDStatus(0x07);
            break;

        case 2:
            status = setLEDStatus(0xC0) && setLEDStatus(0xCA) && setLEDStatus(0xC7);
            break;

        case 3:
//            if (SPS == 3) {
//                setBeepStatus(0x03);
//            } else {
//                if (SPS > 3)
//                    setBeepStatus(0x07);
//                else
//                    setBeepStatus(0x04);
//                setLEDStatus(0x80);
//                setLEDStatus(0x8A);
//            }
            status = setLEDStatus(0xC7) && setLEDStatus(0xC0) && setLEDStatus(0xCA);
            break;

        case 4:
//            setBeepStatus(0x03);
            status = setLEDStatus(0xC7) && setLEDStatus(0xC0) && setLEDStatus(0xCA);
            break;

        default:
            AlwaysLog(valueInvalid, SSTPrompt);
            return false;
    }

    if (status)
        DebugLog(toggleSuccess, SSTPrompt, value, property[value]);
    else
        AlwaysLog(toggleFailure, SSTPrompt);
    return status;
}

bool ThinkVPC::updateKBDLocale(bool update) {
    IOReturn ret = vpc->validateObject(getKBDLang);
    if (ret == kIOReturnNoDevice) {
        return true;
    } else if (ret != kIOReturnSuccess) {
        AlwaysLog(updateFailure, localePrompt);
        return false;
    }

    UInt32 locale;
    if (evaluateIntegerParam(getKBDLang, &locale, 0x02000000) != kIOReturnSuccess) {
        AlwaysLog(updateFailure, localePrompt);
        return false;
    }

    if (update)
        DebugLog(updateSuccess, localePrompt, locale);
    setProperty(localePrompt, locale, 32);

    return true;
}

bool ThinkVPC::setKBDLocale(UInt32 value) {
    UInt32 result;
    if (evaluateIntegerParam(setKBDLang, &result, (value | BIT(24))) != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, localePrompt);
        return false;
    }

    DebugLog(toggleSuccess, localePrompt, value, "-");
    setProperty(localePrompt, value, 32);
    return true;
}

IOReturn ThinkVPC::setPowerState(unsigned long powerStateOrdinal, IOService * whatDevice) {
    if (super::setPowerState(powerStateOrdinal, whatDevice) != kIOPMAckImplied)
        return kIOReturnInvalid;

    if (powerStateOrdinal == 0) {
        if (automaticBacklightMode & BIT(2))
            setSSTStatus(3);
        if ((automaticBacklightMode & BIT(3))) {
            muteLEDstateSaved = muteLEDstate;
            if (muteLEDstate)
                setMuteLEDStatus(false);
        }
        if (automaticBacklightMode & BIT(4)) {
            micMuteLEDstateSaved = micMuteLEDstate;
            if (micMuteLEDstate)
                setMicMuteLEDStatus(0);
        }
    } else {
        if (automaticBacklightMode & BIT(2))
            setSSTStatus(1);
        if ((automaticBacklightMode & BIT(3)) && muteLEDstateSaved)
            setMuteLEDStatus(true);
        if ((automaticBacklightMode & BIT(4)) && micMuteLEDstateSaved)
            setMicMuteLEDStatus(micMuteLEDstateSaved);
    }

    return kIOPMAckImplied;
}
