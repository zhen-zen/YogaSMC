//  SPDX-License-Identifier: GPL-2.0-only
//
//  IdeaVPC.cpp
//  YogaSMC
//
//  Created by Zhen on 2020/7/11.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "IdeaVPC.hpp"
OSDefineMetaClassAndStructors(IdeaVPC, YogaVPC);

void IdeaVPC::updateAll() {
    updateBattery();
    updateKeyboard();
    updateVPC();
}

bool IdeaVPC::initVPC() {
    super::initVPC();
    if (vpc->evaluateInteger(getVPCConfig, &config) != kIOReturnSuccess) {
        AlwaysLog(initFailure, getVPCConfig);
        return false;
    }

    DebugLog(updateSuccess, VPCPrompt, config);
#ifdef DEBUG
    setProperty(VPCPrompt, config, 32);
#endif
    cap_graphics = config >> CFG_GRAPHICS_BIT & 0x7;
    cap_bt       = config >> CFG_BT_BIT & 0x1;
    cap_3g       = config >> CFG_3G_BIT & 0x1;
    cap_wifi     = config >> CFG_WIFI_BIT & 0x1;
    cap_camera   = config >> CFG_CAMERA_BIT & 0x1;

#ifdef DEBUG
    OSDictionary *capabilities = OSDictionary::withCapacity(5);
    OSString *value;

    switch (cap_graphics) {
        case 1:
            setPropertyString(capabilities, "Graphics", "Intel");
            break;

        case 2:
            setPropertyString(capabilities, "Graphics", "ATI");
            break;

        case 3:
            setPropertyString(capabilities, "Graphics", "Nvidia");
            break;

        case 4:
            setPropertyString(capabilities, "Graphics", "Intel and ATI");
            break;

        case 5:
            setPropertyString(capabilities, "Graphics", "Intel and Nvidia");
            break;

        default:
            char Unknown[10];
            snprintf(Unknown, 10, "Unknown:%1x", cap_graphics);
            setPropertyString(capabilities, "Graphics", Unknown);
            break;
    }
    setPropertyBoolean(capabilities, "Bluetooth", cap_bt);
    setPropertyBoolean(capabilities, "3G", cap_3g);
    setPropertyBoolean(capabilities, "Wireless", cap_wifi);
    setPropertyBoolean(capabilities, "Camera", cap_camera);
    setProperty("Capability", capabilities);
    capabilities->release();
#endif
    initEC();
    return true;
}

IOReturn IdeaVPC::message(UInt32 type, IOService *provider, void *argument) {
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

        case kSMC_YogaEvent:
            DebugLog("message: %s Yoga mode 0x%x\n", provider->getName(), *((UInt32 *) argument));
            if (backlightCap && automaticBacklightMode & BIT(1)) {
                updateKeyboard();
                if (*((UInt32 *) argument) != 1) {
                    backlightLevelSaved = backlightLevel;
                    if (backlightLevel)
                        setBacklight(0);
                } else {
                    if (backlightLevelSaved != backlightLevel)
                        setBacklight(backlightLevelSaved);
                }
            }
            break;

        case kSMC_FnlockEvent:
            DebugLog("message: %s Fnlock event\n", provider->getName());
            updateKeyboard();
            toggleFnlock();
            break;

        case kSMC_getConservation:
            *(bool *)argument = conservationMode;
//            conservationModeLock = false;
            DebugLog("message: %s get conservation mode %d\n", provider->getName(), conservationMode);
            break;

        case kSMC_setConservation:
            AlwaysLog("message: %s set conservation mode %d -> %d\n", provider->getName(), conservationMode, *((bool *) argument));
            if (*((bool *) argument) != conservationMode)
                toggleConservation();
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

void IdeaVPC::setPropertiesGated(OSObject *props) {
    OSDictionary *dict = OSDynamicCast(OSDictionary, props);
    if (!dict)
        return;

//    AlwaysLog("%d objects in properties\n", dict->getCount());
    OSCollectionIterator* i = OSCollectionIterator::withCollection(dict);

    if (i) {
        while (OSString* key = OSDynamicCast(OSString, i->getNextObject())) {
            if (key->isEqualTo(conservationPrompt)) {
                OSBoolean *value;
                getPropertyBoolean(conservationPrompt);
                updateBattery(false);

                if (value->getValue() == conservationMode)
                    DebugLog(valueMatched, conservationPrompt, conservationMode);
                else
                    toggleConservation();
            } else if (key->isEqualTo(rapidChargePrompt)) {
                OSBoolean *value;
                getPropertyBoolean(rapidChargePrompt);
                updateBattery(false);

                if (value->getValue() == rapidChargeMode)
                    DebugLog(valueMatched, rapidChargePrompt, rapidChargeMode);
                else
                    toggleRapidCharge();
            } else if (key->isEqualTo(FnKeyPrompt)) {
                OSBoolean *value;
                getPropertyBoolean(FnKeyPrompt);
                updateKeyboard(false);

                if (value->getValue() == FnlockMode)
                    DebugLog(valueMatched, FnKeyPrompt, FnlockMode);
                else
                    toggleFnlock();
            } else if (key->isEqualTo(ECLockPrompt)) {
                OSBoolean *value;
                getPropertyBoolean(ECLockPrompt);
                if (value->getValue())
                    continue;
                
                ECLock = false;
                AlwaysLog("direct VPC EC manipulation is unlocked\n");
            } else if (key->isEqualTo(readECPrompt)) {
                if (ECLock)
                    continue;
                OSNumber *value;
                getPropertyNumber(readECPrompt);

                UInt32 result;
                UInt8 retries = 0;

                if (read_ec_data(value->unsigned32BitValue(), &result, &retries))
                    AlwaysLog("%s 0x%x result: 0x%x %d\n", readECPrompt, value->unsigned32BitValue(), result, retries);
                else
                    AlwaysLog("%s failed 0x%x %d\n", readECPrompt, value->unsigned32BitValue(), retries);
            } else if (key->isEqualTo(writeECPrompt)) {
                if (ECLock)
                    continue;
                OSNumber *value;
                getPropertyNumber(writeECPrompt);

                UInt32 command, data;
                UInt8 retries = 0;
                command = value->unsigned32BitValue() >> 8;
                data = value->unsigned32BitValue() & 0xff;

                if (write_ec_data(command, data, &retries))
                    AlwaysLog("%s 0x%x 0x%x success %d\n", writeECPrompt, command, data, retries);
                else
                    AlwaysLog("%s 0x%x 0x%x failed %d\n", writeECPrompt, command, data, retries);
            } else if (key->isEqualTo(batteryPrompt)) {
                updateBatteryStats();
            } else if (key->isEqualTo(updatePrompt)) {
                updateAll();
                super::updateAll();
            } else if (key->isEqualTo(resetPrompt)) {
                conservationModeLock = false;
                initEC();
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

bool IdeaVPC::initEC() {
    UInt32 state, attempts = 0;
    do {
        if (vpc->evaluateInteger(getKeyboardMode, &state) == kIOReturnSuccess)
            break;
        if (++attempts > 5) {
            AlwaysLog(updateFailure, getKeyboardMode);
            setProperty("EC Access", "Error");
            return false;
        }
        IOSleep(200);
    } while (true);

    if (attempts)
        setProperty("EC Access", attempts + 1, 8);

    backlightCap = BIT(HA_BACKLIGHT_CAP_BIT) & state;
    if (!backlightCap)
        setProperty(backlightPrompt, "unsupported");
    else
        setProperty(autoBacklightPrompt, automaticBacklightMode, 8);

    FnlockCap = BIT(HA_FNLOCK_CAP_BIT) & state;
    if (!FnlockCap)
        setProperty(FnKeyPrompt, "unsupported");

    if (BIT(HA_PRIMEKEY_BIT) & state)
        setProperty("PrimeKeyType", "HotKey");
    else
        setProperty("PrimeKeyType", "FnKey");

    updateBatteryStats();
    return true;
}

void IdeaVPC::updateBatteryStats() {
    OSDictionary *bat0 = OSDictionary::withCapacity(4);
    OSDictionary *bat1 = OSDictionary::withCapacity(4);
    if (updateBatteryID(bat0, bat1) && updateBatteryInfo(bat0, bat1)) {
        if (bat0->getCount())
            setProperty("Battery 0", bat0);
        if (bat1->getCount())
            setProperty("Battery 1", bat1);
    }
    bat0->release();
    bat1->release();
}

bool IdeaVPC::updateBatteryID(OSDictionary *bat0, OSDictionary *bat1) {
    OSObject *result;

    if (vpc->evaluateObject(getBatteryID, &result) != kIOReturnSuccess) {
        AlwaysLog(updateFailure, "Battery ID");
        return false;
    }

    OSArray *data;
    data = OSDynamicCast(OSArray, result);
    if (!data) {
        AlwaysLog("Battery ID not OSArray\n");
        result->release();
        return false;
    }

    OSString *value;
    OSData *cycle0 = OSDynamicCast(OSData, data->getObject(0));
    if (!cycle0) {
        AlwaysLog("Battery 0 cycle not exist\n");
    } else {
        UInt16 *cycleCount0 = (UInt16 *)cycle0->getBytesNoCopy();
        if (cycleCount0[0] != 0xffff) {
            char cycle0String[5];
            snprintf(cycle0String, 5, "%d", cycleCount0[0]);
            setPropertyString(bat0, "Cycle count", cycle0String);
            AlwaysLog("Battery 0 cycle count %d\n", cycleCount0[0]);
        }
    }

    OSData *cycle1 = OSDynamicCast(OSData, data->getObject(1));
    if (!cycle1) {
        DebugLog("Battery 1 cycle not exist\n");
    } else {
        UInt16 *cycleCount1 = (UInt16 *)cycle1->getBytesNoCopy();
        if (cycleCount1[0] != 0xffff) {
            char cycle1String[5];
            snprintf(cycle1String, 5, "%d", cycleCount1[0]);
            setPropertyString(bat1, "Cycle count", cycle1String);
            AlwaysLog("Battery 1 cycle count %d\n", cycleCount1[0]);
        }
    }

    OSData *ID0 = OSDynamicCast(OSData, data->getObject(2));
    if (!ID0) {
        AlwaysLog("Battery 0 ID not exist\n");
    } else {
        UInt16 *batteryID0 = (UInt16 *)ID0->getBytesNoCopy();
        if (batteryID0[0] != 0xffff) {
            char ID0String[20];
            snprintf(ID0String, 20, "%04x %04x %04x %04x", batteryID0[0], batteryID0[1], batteryID0[2], batteryID0[3]);
            setPropertyString(bat0, "ID", ID0String);
            AlwaysLog("Battery 0 ID %s\n", ID0String);
        }
    }

    OSData *ID1 = OSDynamicCast(OSData, data->getObject(3));
    if (!ID1) {
        DebugLog("Battery 1 ID not exist\n");
    } else {
        UInt16 *batteryID1 = (UInt16 *)ID1->getBytesNoCopy();
        if (batteryID1[0] != 0xffff) {
            char ID1String[20];
            snprintf(ID1String, 20, "%04x %04x %04x %04x", batteryID1[0], batteryID1[1], batteryID1[2], batteryID1[3]);
            setPropertyString(bat1, "ID", ID1String);
            AlwaysLog("Battery 1 ID %s\n", ID1String);
        }
    }
    data->release();
    return true;
}

bool IdeaVPC::updateBatteryInfo(OSDictionary *bat0, OSDictionary *bat1) {
    OSData *data;
    OSObject *result;
    
    OSObject* params[1] = {
        OSNumber::withNumber(0ULL, 32)
    };

    if (vpc->evaluateObject(getBatteryInfo, &result, params, 1) != kIOReturnSuccess) {
        AlwaysLog(updateFailure, "Battery Info");
        return false;
    }

    data = OSDynamicCast(OSData, result);
    if (!data) {
        AlwaysLog("Battery Info not OSData\n");
        result->release();
        return false;
    }

    OSString *value;
    UInt16 * bdata = (UInt16 *)(data->getBytesNoCopy());
    // B1TM
    if (bdata[7] != 0) {
        SInt16 celsius = bdata[7] - 2731;
        if (celsius < 0 || celsius > 600)
            AlwaysLog("Battery 0 temperature critical! %d\n", celsius);
        char temperature0[9];
        snprintf(temperature0, 9, "%d.%d℃", celsius / 10, celsius % 10);
        setPropertyString(bat0, "Temperature", temperature0);
//        UInt16 fahrenheit = celsius * 9 / 5 + 320;
//        snprintf(temperature0, 9, "%d.%d℉", fahrenheit / 10, fahrenheit % 10);
//        setPropertyString(bat0, "Fahrenheit", temperature0);
    }
    // B1DT
    if (bdata[8] != 0) {
        char date0[11];
        snprintf(date0, 11, "%4d/%02d/%02d", (bdata[8] >> 9) + 1980, (bdata[8] >> 5) & 0x0f, bdata[8] & 0x1f);
        setPropertyString(bat0, "Manufacture date", date0);
    }
    // B2DT
    if (bdata[9] != 0) {
        char date1[11];
        snprintf(date1, 11, "%4d/%02d/%02d", (bdata[9] >> 9) + 1980, (bdata[9] >> 5) & 0x0f, bdata[9] & 0x1f);
        setPropertyString(bat0, "Manufacture date", date1);
    }
    data->release();
    return true;
}

bool IdeaVPC::updateBattery(bool update) {
    UInt32 state;

    if (vpc->evaluateInteger(getBatteryMode, &state) != kIOReturnSuccess) {
        AlwaysLog(updateFailure, batteryPrompt);
        return false;
    }

    conservationMode = BIT(BM_CONSERVATION_BIT) & state;
    rapidChargeMode = BIT(BM_RAPIDCHARGE_BIT) & state;

    if (update) {
        DebugLog(updateSuccess, batteryPrompt, state);
        setProperty(conservationPrompt, conservationMode);
        setProperty(rapidChargePrompt, rapidChargeMode);
    }

    return true;
}

bool IdeaVPC::updateKeyboard(bool update) {
    if (!FnlockCap && !backlightCap)
        return true;

    UInt32 state;

    if (vpc->evaluateInteger(getKeyboardMode, &state) != kIOReturnSuccess) {
        AlwaysLog(updateFailure, KeyboardPrompt);
        return false;
    }

    if (FnlockCap)
        FnlockMode = BIT(HA_FNLOCK_BIT) & state;
    if (backlightCap)
        backlightLevel = (BIT(HA_BACKLIGHT_BIT) & state) ? 1 : 0;

    if (update) {
        DebugLog(updateSuccess, KeyboardPrompt, state);
        if (FnlockCap)
            setProperty(FnKeyPrompt, FnlockMode);
        if (backlightCap)
            setProperty(backlightPrompt, backlightLevel, 32);
    }

    return true;
}

bool IdeaVPC::toggleConservation() {
    if (conservationModeLock)
        return false;

    UInt32 result;

    OSObject* params[1] = {
        OSNumber::withNumber((!conservationMode ? BMCMD_CONSERVATION_ON : BMCMD_CONSERVATION_OFF), 32)
    };

    if (vpc->evaluateInteger(setBatteryMode, &result, params, 1) != kIOReturnSuccess || result != 0) {
        AlwaysLog(toggleFailure, conservationPrompt);
        return false;
    }

    conservationMode = !conservationMode;
    DebugLog(toggleSuccess, conservationPrompt, (conservationMode ? BMCMD_CONSERVATION_ON : BMCMD_CONSERVATION_OFF), (conservationMode ? "on" : "off"));
    setProperty(conservationPrompt, conservationMode);

    return true;
}

bool IdeaVPC::toggleRapidCharge() {
    UInt32 result;

    OSObject* params[1] = {
        OSNumber::withNumber((!rapidChargeMode ? BMCMD_RAPIDCHARGE_ON : BMCMD_RAPIDCHARGE_OFF), 32)
    };

    if (vpc->evaluateInteger(setBatteryMode, &result, params, 1) != kIOReturnSuccess || result != 0) {
        AlwaysLog(toggleFailure, rapidChargePrompt);
        return false;
    }

    rapidChargeMode = !rapidChargeMode;
    DebugLog(toggleSuccess, rapidChargePrompt, (rapidChargeMode ? BMCMD_RAPIDCHARGE_ON : BMCMD_RAPIDCHARGE_OFF), (rapidChargeMode ? "on" : "off"));
    setProperty(rapidChargePrompt, conservationMode);

    return true;
}

bool IdeaVPC::setBacklight(UInt32 level) {
    if (!backlightCap)
        return true;

    UInt32 result;

    OSObject* params[1] = {
        OSNumber::withNumber((level ? HACMD_BACKLIGHT_ON : HACMD_BACKLIGHT_OFF), 32)
    };

    if (vpc->evaluateInteger(setKeyboardMode, &result, params, 1) != kIOReturnSuccess || result != 0) {
        AlwaysLog(toggleFailure, backlightPrompt);
        return false;
    }

    backlightLevel = level;
    DebugLog(toggleSuccess, backlightPrompt, (backlightLevel ? HACMD_BACKLIGHT_ON : HACMD_BACKLIGHT_OFF), (backlightLevel ? "on" : "off"));
    setProperty(backlightPrompt, backlightLevel);

    return true;
}

bool IdeaVPC::toggleFnlock() {
    if (!FnlockCap)
        return true;

    UInt32 result;

    OSObject* params[1] = {
        OSNumber::withNumber((!FnlockMode ? HACMD_FNLOCK_ON : HACMD_FNLOCK_OFF), 32)
    };

    if (vpc->evaluateInteger(setKeyboardMode, &result, params, 1) != kIOReturnSuccess || result != 0) {
        AlwaysLog(toggleFailure, FnKeyPrompt);
        return false;
    }

    FnlockMode = !FnlockMode;
    DebugLog(toggleSuccess, FnKeyPrompt, (FnlockMode ? HACMD_FNLOCK_ON : HACMD_FNLOCK_OFF), (FnlockMode ? "on" : "off"));
    setProperty(FnKeyPrompt, FnlockMode);

    return true;
}

void IdeaVPC::updateVPC() {
    UInt32 vpc1, vpc2, result;
    UInt8 retries = 0;

    if (!read_ec_data(VPCCMD_R_VPC1, &vpc1, &retries) || !read_ec_data(VPCCMD_R_VPC2, &vpc2, &retries)) {
        AlwaysLog("Failed to read VPC %d\n", retries);
        return;
    }

    vpc1 = (vpc2 << 8) | vpc1;
#ifdef DEBUG
    AlwaysLog("read VPC EC result: 0x%x %d\n", vpc1, retries);
    setProperty("VPCstatus", vpc1, 32);
#endif
    for (int vpc_bit = 0; vpc_bit < 16; vpc_bit++) {
        if (BIT(vpc_bit) & vpc1) {
            switch (vpc_bit) {
                case 0:
                    if (!read_ec_data(VPCCMD_R_SPECIAL_BUTTONS, &result, &retries)) {
                        AlwaysLog("Failed to read VPCCMD_R_SPECIAL_BUTTONS %d\n", retries);
                    } else {
                        switch (result) {
                            case 0x40:
                                AlwaysLog("Fn+Q cooling\n");
                                // TODO: fan status switch
                                break;

                            default:
                                AlwaysLog("Special button 0x%x\n", result);
                                break;
                        }
                    }
                    break;

                case 1: // ENERGY_EVENT_GENERAL / ENERGY_EVENT_KEYBDLED_OLD
                    AlwaysLog("Fn+Space keyboard backlight?\n");
                    updateKeyboard();
                    // also on AC connect / disconnect
                    break;

                case 2:
                    if (!read_ec_data(VPCCMD_R_BL_POWER, &result, &retries))
                        AlwaysLog("Failed to read VPCCMD_R_BL_POWER %d\n", retries);
                    else
                        AlwaysLog("Open lid? 0x%x %s\n", result, result ? "on" : "off");
                    // functional, TODO: turn off screen on demand
                    break;

                case 5:
                    if (!read_ec_data(VPCCMD_R_TOUCHPAD, &result, &retries))
                        AlwaysLog("Failed to read VPCCMD_R_TOUCHPAD %d\n", retries);
                    else
                        AlwaysLog("Fn+F6 touchpad 0x%x %s\n", result, result ? "on" : "off");
                    // functional, TODO: manually toggle
                    break;

                case 7:
                    AlwaysLog("Fn+F8 camera\n");
                    // TODO: camera status switch
                    break;

                case 8: // ENERGY_EVENT_MIC
                    AlwaysLog("Fn+F4 mic\n");
                    // TODO: mic status switch
                    break;

                case 10:
                    AlwaysLog("Fn+F6 touchpad on\n");
                    // functional, identical to case 5?
                    break;

                case 12: // ENERGY_EVENT_KEYBDLED
                    AlwaysLog("Fn+Space keyboard backlight?\n");
                    break;

                case 13:
                    AlwaysLog("Fn+F7 airplane mode\n");
                    // TODO: airplane mode switch
                    break;

                default:
                    AlwaysLog("Unknown VPC event %d\n", vpc_bit);
                    break;
            }
        }
    }
}

bool IdeaVPC::read_ec_data(UInt32 cmd, UInt32 *result, UInt8 *retries) {
    if (!method_vpcw(1, cmd))
        return false;

    AbsoluteTime abstime, deadline, now_abs;
    nanoseconds_to_absolutetime(IDEAPAD_EC_TIMEOUT * 1000 * 1000, &abstime);
    clock_absolutetime_interval_to_deadline(abstime, &deadline);

    do {
        if (!method_vpcr(1, result))
            return false;

        if (*result == 0)
            return method_vpcr(0, result);

        *retries = *retries + 1;
        IODelay(250);
        clock_get_uptime(&now_abs);
    } while (now_abs < deadline || *retries < 5);

    AlwaysLog(timeoutPrompt, readECPrompt, cmd);
    return false;
}

bool IdeaVPC::write_ec_data(UInt32 cmd, UInt32 value, UInt8 *retries) {
    UInt32 result;

    if (!method_vpcw(0, value))
        return false;

    if (!method_vpcw(1, cmd))
        return false;

    AbsoluteTime abstime, deadline, now_abs;
    nanoseconds_to_absolutetime(IDEAPAD_EC_TIMEOUT * 1000 * 1000, &abstime);
    clock_absolutetime_interval_to_deadline(abstime, &deadline);

    do {
        if (!method_vpcr(1, &result))
            return false;

        if (result == 0)
            return true;

        *retries = *retries + 1;
        IODelay(250);
        clock_get_uptime(&now_abs);
    } while (now_abs < deadline || *retries < 5);

    AlwaysLog(timeoutPrompt, writeECPrompt, cmd);
    return false;
}

bool IdeaVPC::method_vpcr(UInt32 cmd, UInt32 *result) {
    OSObject* params[1] = {
        OSNumber::withNumber(cmd, 32)
    };

    return (vpc->evaluateInteger(readVPCStatus, result, params, 1) == kIOReturnSuccess);
}

bool IdeaVPC::method_vpcw(UInt32 cmd, UInt32 data) {
    UInt32 result; // should only return 0

    OSObject* params[2] = {
        OSNumber::withNumber(cmd, 32),
        OSNumber::withNumber(data, 32)
    };

    return (vpc->evaluateInteger(writeVPCStatus, &result, params, 2) == kIOReturnSuccess);
}
