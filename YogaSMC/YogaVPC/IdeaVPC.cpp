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
    if (!initEC())
        return false;

    super::initVPC();

    UInt32 config;
    if (vpc->evaluateInteger(getVPCConfig, &config) != kIOReturnSuccess) {
        AlwaysLog(initFailure, getVPCConfig);
        return false;
    }

    DebugLog(updateSuccess, VPCPrompt, config);
    setProperty(VPCPrompt, config, 32);

    OSDictionary *capabilities = OSDictionary::withCapacity(6);
    OSString *value;

    setPropertyBoolean(capabilities, "Bluetooth", (config >> CFG_BT_BIT) & 0x1);
    setPropertyBoolean(capabilities, "3G", (config >> CFG_3G_BIT) & 0x1);
    setPropertyBoolean(capabilities, "Wireless", (config >> CFG_WIFI_BIT) & 0x1);
    setPropertyBoolean(capabilities, "Camera", (config >> CFG_CAMERA_BIT) & 0x1);
    setPropertyBoolean(capabilities, "Touchpad", (config >> CFG_TOUCHPAD_BIT) & 0x1);

    UInt8 cap_graphics = config >> CFG_GRAPHICS_BIT & 0x7;
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
    setProperty("Capability", capabilities);
    capabilities->release();

    if (vpc->evaluateInteger(getKeyboardMode, &config) == kIOReturnSuccess)
        updateKeyboardCapability(config);
    else
        AlwaysLog(updateFailure, keyboardPrompt);

    if (vpc->evaluateInteger(getBatteryMode, &config) == kIOReturnSuccess)
        updateBatteryCapability(config);
    else
        AlwaysLog(updateFailure, batteryPrompt);

    if (checkKernelArgument("-ideabr")) {
        brightnessPoller = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &IdeaVPC::brightnessAction));
        if (!brightnessPoller ||
            workLoop->addEventSource(brightnessPoller) != kIOReturnSuccess) {
            AlwaysLog("Failed to add brightnessPoller");
            OSSafeReleaseNULL(brightnessPoller);
        } else {
            brightnessPoller->setTimeoutMS(BR_POLLING_INTERVAL);
            brightnessPoller->enable();
            AlwaysLog("BrightnessPoller enabled");
        }
    }
    return true;
}

void IdeaVPC::brightnessAction(OSObject *owner, IOTimerEventSource *timer) {
    updateVPC();
    if (brightnessPoller->setTimeoutMS(BR_POLLING_INTERVAL) != kIOReturnSuccess)
        AlwaysLog("Failed to set poller");
    else
        DebugLog("Poller set");
}

bool IdeaVPC::exitVPC() {
    if (brightnessPoller) {
        brightnessPoller->disable();
        workLoop->removeEventSource(brightnessPoller);
    }
    OSSafeReleaseNULL(brightnessPoller);
    return super::exitVPC();
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
        case kSMC_notifyKeystroke:
            break;

        case kSMC_YogaEvent:
            {
                UInt32 mode = *((UInt32 *) argument);
                DebugLog("message: %s Yoga mode 0x%x", provider->getName(), mode);
                if (client)
                    client->sendNotification(0x10, mode); // Since vpc_bit is less than 16
                if (backlightCap && automaticBacklightMode & BIT(1)) {
                    updateKeyboard();
                    if (mode != 1) {
                        backlightLevelSaved = backlightLevel;
                        if (backlightLevel)
                            setBacklight(0);
                    } else {
                        if (backlightLevelSaved != backlightLevel)
                            setBacklight(backlightLevelSaved);
                    }
                }
            }
            break;

        case kSMC_FnlockEvent:
            DebugLog("message: %s Fnlock event", provider->getName());
            updateKeyboard();
            toggleFnlock();
            break;

        case kSMC_getConservation:
            *(bool *)argument = conservationMode;
//            conservationModeLock = false;
            DebugLog("message: %s get conservation mode %d", provider->getName(), conservationMode);
            break;

        case kSMC_setConservation:
            AlwaysLog("message: %s set conservation mode %d -> %d", provider->getName(), conservationMode, *((bool *) argument));
            if (*((bool *) argument) != conservationMode &&
                !conservationModeLock)
                toggleConservation();
            break;

        case kIOACPIMessageDeviceNotification:
            if (!argument)
                AlwaysLog("message: Unknown ACPI notification");
            else if (*((UInt32 *) argument) == kIOACPIMessageReserved)
                updateVPC();
            else
                AlwaysLog("message: Unknown ACPI notification 0x%04x", *((UInt32 *) argument));
            break;

        default:
            if (argument)
                AlwaysLog("message: type=%x, provider=%s, argument=0x%04x", type, provider->getName(), *((UInt32 *) argument));
            else
                AlwaysLog("message: type=%x, provider=%s", type, provider->getName());
    }

    return kIOReturnSuccess;
}

void IdeaVPC::setPropertiesGated(OSObject *props) {
    OSDictionary *dict = OSDynamicCast(OSDictionary, props);
    if (!dict)
        return;

//    AlwaysLog("%d objects in properties", dict->getCount());
    OSCollectionIterator* i = OSCollectionIterator::withCollection(dict);

    if (i) {
        while (OSString* key = OSDynamicCast(OSString, i->getNextObject())) {
            if (key->isEqualTo(alwaysOnUSBPrompt)) {
                OSBoolean *value;
                getPropertyBoolean(alwaysOnUSBPrompt);
                updateKeyboard(false);

                if (value->getValue() == alwaysOnUSBMode)
                    DebugLog(valueMatched, alwaysOnUSBPrompt, alwaysOnUSBMode);
                else
                    toggleAlwaysOnUSB();
            } else if (key->isEqualTo(conservationPrompt)) {
#ifdef DEBUG
                OSNumber *raw = OSDynamicCast(OSNumber, dict->getObject(conservationPrompt));
                if (raw != nullptr) {
                    UInt32 result;

                    OSObject* params[1] = {
                        OSNumber::withNumber(raw->unsigned8BitValue(), 32)
                    };

                    if (vpc->evaluateInteger(setBatteryMode, &result, params, 1) != kIOReturnSuccess || result != 0)
                        AlwaysLog(toggleFailure, conservationPrompt);
                    else
                        AlwaysLog(toggleSuccess, conservationPrompt, raw->unsigned32BitValue(), "see ioreg");
                    params[0]->release();
                    continue;
                }
#endif
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
                AlwaysLog("direct VPC EC manipulation is unlocked");
            } else if (key->isEqualTo(readECPrompt)) {
                if (ECLock)
                    continue;
                OSNumber *value;
                getPropertyNumber(readECPrompt);

                UInt32 result;
                UInt32 retries = 0;

                if (read_ec_data(value->unsigned32BitValue(), &result, &retries))
                    AlwaysLog("%s 0x%x result: 0x%x %d", readECPrompt, value->unsigned32BitValue(), result, retries);
                else
                    AlwaysLog("%s failed 0x%x %d", readECPrompt, value->unsigned32BitValue(), retries);
            } else if (key->isEqualTo(writeECPrompt)) {
                if (ECLock)
                    continue;
                OSNumber *value;
                getPropertyNumber(writeECPrompt);

                UInt32 command, data;
                UInt32 retries = 0;
                command = value->unsigned32BitValue() >> 8;
                data = value->unsigned32BitValue() & 0xff;

                if (write_ec_data(command, data, &retries))
                    AlwaysLog("%s 0x%x 0x%x success %d", writeECPrompt, command, data, retries);
                else
                    AlwaysLog("%s 0x%x 0x%x failed %d", writeECPrompt, command, data, retries);
            } else if (key->isEqualTo(batteryPrompt)) {
                UInt32 state;
                if (vpc->evaluateInteger(getBatteryMode, &state) == kIOReturnSuccess)
                    updateBatteryStats(state);
                else
                    AlwaysLog(updateFailure, batteryPrompt);
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

    // _REG will write Arg1 to ECAV (or OKEC on old machines) to connect / disconnect the region
    if (ec->validateObject(getECStatus) == kIOReturnSuccess) {
        do {
            if (ec->evaluateInteger(getECStatus, &state) == kIOReturnSuccess && state != 0) {
                if (attempts != 0)
                    setProperty("EC Access Retries", attempts, 8);
                return true;
            }
            IOSleep(100);
        } while (++attempts < 5);
        AlwaysLog(updateFailure, getECStatus);
    }

    if (ec->validateObject(getECStatusLegacy) == kIOReturnSuccess) {
        do {
            if (ec->evaluateInteger(getECStatusLegacy, &state) == kIOReturnSuccess && state != 0) {
                if (attempts != 0)
                    setProperty("EC Access Retries", attempts, 8);
                return true;
            }
            IOSleep(100);
        } while (++attempts < 5);
        AlwaysLog(updateFailure, getECStatusLegacy);
    }

    // fallback to battery mode since some laptop don't have available keyboard mode
    do {
        if (vpc->evaluateInteger(getBatteryMode, &state) == kIOReturnSuccess) {
            setProperty("EC Access Retries", attempts, 8);
            return true;
        }
        IOSleep(100);
    } while (++attempts < 10);
    AlwaysLog(initFailure, getBatteryMode);
    return false;
}

void IdeaVPC::updateKeyboardCapability(UInt32 kbdState) {
    backlightCap = BIT(HA_BACKLIGHT_CAP_BIT) & kbdState;
    if (!backlightCap)
        setProperty(backlightPrompt, "unsupported");
    else
        setProperty(autoBacklightPrompt, automaticBacklightMode, 8);

    FnlockCap = BIT(HA_FNLOCK_CAP_BIT) & kbdState;
    if (!FnlockCap)
        setProperty(FnKeyPrompt, "unsupported");

    if (BIT(HA_PRIMEKEY_BIT) & kbdState)
        setProperty("PrimeKeyType", "HotKey");
    else
        setProperty("PrimeKeyType", "FnKey");

    alwaysOnUSBCap = BIT(HA_AOUSB_CAP_BIT) & kbdState;
    if (!alwaysOnUSBCap)
        setProperty(alwaysOnUSBPrompt, "unsupported");
}

void IdeaVPC::updateBatteryCapability(UInt32 batState) {
    OSDictionary *bat0 = OSDictionary::withCapacity(5);
    OSDictionary *bat1 = OSDictionary::withCapacity(5);
    if (BIT(BM_BATTERY0BAD_BIT) & batState) {
        bat0->setObject("Critical", kOSBooleanTrue);
        setProperty("Battery", "Critical");
        AlwaysLog("Battery 0 critical!");
        conservationModeLock = true;
    }

    if (BIT(BM_BATTERY1BAD_BIT) & batState) {
        bat1->setObject("Critical", kOSBooleanTrue);
        setProperty("Battery", "Critical");
        AlwaysLog("Battery 1 critical!");
        conservationModeLock = true;
    }

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
        AlwaysLog("Battery ID not OSArray");
        result->release();
        return false;
    }

    OSString *value;
    OSData *cycle0 = OSDynamicCast(OSData, data->getObject(0));
    if (!cycle0) {
        AlwaysLog("Battery 0 cycle not exist");
    } else {
        UInt16 *cycleCount0 = (UInt16 *)cycle0->getBytesNoCopy();
        if (cycleCount0[0] != 0xffff) {
            char cycle0String[5];
            snprintf(cycle0String, 5, "%d", cycleCount0[0]);
            setPropertyString(bat0, "Cycle count", cycle0String);
            DebugLog("Battery 0 cycle count %d", cycleCount0[0]);
        }
    }

    OSData *cycle1 = OSDynamicCast(OSData, data->getObject(1));
    if (!cycle1) {
        DebugLog("Battery 1 cycle not exist");
    } else {
        UInt16 *cycleCount1 = (UInt16 *)cycle1->getBytesNoCopy();
        if (cycleCount1[0] != 0xffff) {
            char cycle1String[5];
            snprintf(cycle1String, 5, "%d", cycleCount1[0]);
            setPropertyString(bat1, "Cycle count", cycle1String);
            DebugLog("Battery 1 cycle count %d", cycleCount1[0]);
        }
    }

    OSData *ID0 = OSDynamicCast(OSData, data->getObject(2));
    if (!ID0) {
        AlwaysLog("Battery 0 ID not exist");
    } else {
        UInt16 *batteryID0 = (UInt16 *)ID0->getBytesNoCopy();
        if (batteryID0[0] != 0xffff) {
            char ID0String[20];
            snprintf(ID0String, 20, "%04x %04x %04x %04x", batteryID0[0], batteryID0[1], batteryID0[2], batteryID0[3]);
            setPropertyString(bat0, "ID", ID0String);
            DebugLog("Battery 0 ID %s", ID0String);
        }
    }

    OSData *ID1 = OSDynamicCast(OSData, data->getObject(3));
    if (!ID1) {
        DebugLog("Battery 1 ID not exist");
    } else {
        UInt16 *batteryID1 = (UInt16 *)ID1->getBytesNoCopy();
        if (batteryID1[0] != 0xffff) {
            char ID1String[20];
            snprintf(ID1String, 20, "%04x %04x %04x %04x", batteryID1[0], batteryID1[1], batteryID1[2], batteryID1[3]);
            setPropertyString(bat1, "ID", ID1String);
            DebugLog("Battery 1 ID %s", ID1String);
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

    IOReturn ret = vpc->evaluateObject(getBatteryInfo, &result, params, 1);
    params[0]->release();

    if (ret != kIOReturnSuccess) {
        AlwaysLog(updateFailure, "Battery Info");
        return false;
    }

    data = OSDynamicCast(OSData, result);
    if (!data) {
        AlwaysLog("Battery Info not OSData");
        result->release();
        return false;
    }

    OSString *value;
    UInt16 * bdata = (UInt16 *)(data->getBytesNoCopy());
    // B1TM
    if (bdata[7] != 0) {
        SInt16 celsius = bdata[7] - 2731;
        if (celsius < 0 || celsius > 600)
            AlwaysLog("Battery 0 temperature critical! %d", celsius);
        char temperature0[9];
        snprintf(temperature0, 9, "%d.%d℃", celsius / 10, celsius % 10);
        setPropertyString(bat0, "Temperature", temperature0);
        DebugLog("Battery 0 temperature %s", temperature0);
//        UInt16 fahrenheit = celsius * 9 / 5 + 320;
//        snprintf(temperature0, 9, "%d.%d℉", fahrenheit / 10, fahrenheit % 10);
//        setPropertyString(bat0, "Fahrenheit", temperature0);
    }
    // B1DT
    if (bdata[8] != 0) {
        char date0[11];
        snprintf(date0, 11, "%4d/%02d/%02d", (bdata[8] >> 9) + 1980, (bdata[8] >> 5) & 0x0f, bdata[8] & 0x1f);
        setPropertyString(bat0, "Manufacture date", date0);
        DebugLog("Battery 0 manufacture date %s", date0);
    }
    // B2DT
    if (bdata[9] != 0) {
        char date1[11];
        snprintf(date1, 11, "%4d/%02d/%02d", (bdata[9] >> 9) + 1980, (bdata[9] >> 5) & 0x0f, bdata[9] & 0x1f);
        setPropertyString(bat1, "Manufacture date", date1);
        DebugLog("Battery 1 manufacture date %s", date1);
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

    if (update)
        DebugLog(updateSuccess, batteryPrompt, state);

    setProperty(conservationPrompt, conservationMode);
    setProperty(rapidChargePrompt, rapidChargeMode);

    return true;
}

bool IdeaVPC::updateKeyboard(bool update) {
    if (!FnlockCap && !backlightCap)
        return true;

    UInt32 state;

    if (vpc->evaluateInteger(getKeyboardMode, &state) != kIOReturnSuccess) {
        AlwaysLog(updateFailure, keyboardPrompt);
        return false;
    }

    if (backlightCap) {
        // Preserve low / high level on non keyboard triggered events
        if (!update && backlightLevel)
            backlightLevel -= 1;
        // Inference from brightness cycle: 0 -> 1 -> 2 -> 0
        backlightLevel = (BIT(HA_BACKLIGHT_BIT) & state) ? (backlightLevel ? 2 : 1) : 0;
    }
    if (FnlockCap)
        FnlockMode = BIT(HA_FNLOCK_BIT) & state;
    if (alwaysOnUSBCap)
        alwaysOnUSBMode = BIT(HA_AOUSB_BIT) & state;

    if (update)
        DebugLog(updateSuccess, keyboardPrompt, state);

    if (backlightCap)
        setProperty(backlightPrompt, backlightLevel, 32);
    if (FnlockCap)
        setProperty(FnKeyPrompt, FnlockMode);
    if (alwaysOnUSBCap)
        setProperty(alwaysOnUSBPrompt, alwaysOnUSBMode);

    return true;
}

bool IdeaVPC::toggleAlwaysOnUSB() {
    if (!alwaysOnUSBCap)
        return true;

    UInt32 result;

    OSObject* params[1] = {
        OSNumber::withNumber((!alwaysOnUSBMode ? HACMD_AOUSB_ON : HACMD_AOUSB_OFF), 32)
    };

    IOReturn ret = vpc->evaluateInteger(setKeyboardMode, &result, params, 1);
    params[0]->release();

    if (ret != kIOReturnSuccess || result != 0) {
        AlwaysLog(toggleFailure, alwaysOnUSBPrompt);
        return false;
    }

    alwaysOnUSBMode = !alwaysOnUSBMode;
    DebugLog(toggleSuccess, alwaysOnUSBPrompt, (alwaysOnUSBMode ? HACMD_AOUSB_ON : HACMD_AOUSB_OFF), (alwaysOnUSBMode ? "on" : "off"));
    setProperty(alwaysOnUSBPrompt, alwaysOnUSBMode);

    return true;
}

bool IdeaVPC::toggleConservation() {
    UInt32 result;

    OSObject* params[1] = {
        OSNumber::withNumber((!conservationMode ? BMCMD_CONSERVATION_ON : BMCMD_CONSERVATION_OFF), 32)
    };

    IOReturn ret = vpc->evaluateInteger(setBatteryMode, &result, params, 1);
    params[0]->release();

    if (ret != kIOReturnSuccess || result != 0) {
        AlwaysLog(toggleFailure, conservationPrompt);
        return false;
    }

    conservationMode = !conservationMode;
    DebugLog(toggleSuccess, conservationPrompt, (conservationMode ? BMCMD_CONSERVATION_ON : BMCMD_CONSERVATION_OFF), (conservationMode ? "on" : "off"));
    setProperty(conservationPrompt, conservationMode);

    notifyBattery();
    return true;
}

bool IdeaVPC::toggleRapidCharge() {
    // experimental, reset required
    if (conservationModeLock)
        return false;

    UInt32 result;

    OSObject* params[1] = {
        OSNumber::withNumber((!rapidChargeMode ? BMCMD_RAPIDCHARGE_ON : BMCMD_RAPIDCHARGE_OFF), 32)
    };

    IOReturn ret = vpc->evaluateInteger(setBatteryMode, &result, params, 1);
    params[0]->release();

    if (ret != kIOReturnSuccess || result != 0) {
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

    IOReturn ret = vpc->evaluateInteger(setKeyboardMode, &result, params, 1);
    params[0]->release();

    if (ret != kIOReturnSuccess || result != 0) {
        AlwaysLog(toggleFailure, backlightPrompt);
        return false;
    }

    if (level && backlightLevel)
        // Preserve low / high level on non keyboard triggered events
        backlightLevel -= 1;
    // Don't update internal value since it will trigger another event
    DebugLog(toggleSuccess, backlightPrompt, (backlightLevel ? HACMD_BACKLIGHT_ON : HACMD_BACKLIGHT_OFF), (backlightLevel ? "on" : "off"));
    return true;
}

bool IdeaVPC::toggleFnlock() {
    if (!FnlockCap)
        return true;

    UInt32 result;

    OSObject* params[1] = {
        OSNumber::withNumber((!FnlockMode ? HACMD_FNLOCK_ON : HACMD_FNLOCK_OFF), 32)
    };

    IOReturn ret = vpc->evaluateInteger(setKeyboardMode, &result, params, 1);
    params[0]->release();

    if (ret != kIOReturnSuccess || result != 0) {
        AlwaysLog(toggleFailure, FnKeyPrompt);
        return false;
    }

    FnlockMode = !FnlockMode;
    DebugLog(toggleSuccess, FnKeyPrompt, (FnlockMode ? HACMD_FNLOCK_ON : HACMD_FNLOCK_OFF), (FnlockMode ? "on" : "off"));
    setProperty(FnKeyPrompt, FnlockMode);

    if (client)
        client->sendNotification(0x11, FnlockMode);
    return true;
}

void IdeaVPC::updateVPC() {
    UInt32 vpc1, vpc2, result;
    UInt32 retries = 0;

    if (!read_ec_data(VPCCMD_R_VPC1, &vpc1, &retries)) {
        AlwaysLog("Failed to read VPC1 after %d attempts", retries);
        return;
    }

    if (!read_ec_data(VPCCMD_R_VPC2, &vpc2, &retries)) {
        AlwaysLog("Failed to read VPC2 after %d attempts", retries);
    } else {
        vpc1 = (vpc2 << 8) | vpc1;
    }

    if (!vpc1) {
        if (!brightnessPoller)
            DebugLog("empty EC event: %d", retries);
        return;
    }

    if (!brightnessPoller)
        DebugLog("read VPC EC result: 0x%x %d", vpc1, retries);

    UInt64 time = 0;
    for (int vpc_bit = 0; vpc_bit < 16; vpc_bit++) {
        if (BIT(vpc_bit) & vpc1) {
            UInt32 data = 0;
            switch (vpc_bit) {
                case 0:
                    if (!read_ec_data(VPCCMD_R_SPECIAL_BUTTONS, &result, &retries)) {
                        AlwaysLog("Failed to read VPCCMD_R_SPECIAL_BUTTONS %d", retries);
                    } else {
                        switch (result) {
                            case 0x01:
                            case 0x40: // KEY_PROG4
                                DebugLog("Fn+Q cooling 0x%x", result);
                                break;

                            case 0x02: // KEY_PROG3
                                DebugLog("OneKey Theater");
                                break;

                            default:
                                AlwaysLog("Special button 0x%x", result);
                                break;
                        }
                        data = result;
                    }
                    time = 1;
                    break;

                case 1: // ENERGY_EVENT_GENERAL / ENERGY_EVENT_KEYBDLED_OLD
                    DebugLog("Fn+Space keyboard backlight old?");
                    updateKeyboard(true);
                    data = backlightLevel;
                    time = 1;
                    // also on AC connect / disconnect
                    break;

                case 2:
                    if (!read_ec_data(VPCCMD_R_BL_POWER, &result, &retries))
                        AlwaysLog("Failed to read VPCCMD_R_BL_POWER %d", retries);
                    else
                        DebugLog("Open lid? 0x%x %s", result, result ? "on" : "off");
                    data = result;
                    // functional, TODO: turn off screen on demand
                    break;

                case 3: // long_pressed ? KEY_PROG2 : KEY_PROG1
                    if (!read_ec_data(VPCCMD_R_NOVO, &result, &retries)) {
                        AlwaysLog("Failed to read VPCCMD_R_NOVO %d", retries);
                    } else {
                        DebugLog("NOVO button 0x%x", result);
                        data = result;
                    }
                    time = 1;
                    break;

                case 4:
                    if (!brightnessPoller) 
                        break;
                    if (!read_ec_data(VPCCMD_R_BL, &result, &retries)) {
                        AlwaysLog("Failed to read VPCCMD_R_BL %d", retries);
                    } else {
                        if (result == 0 || result < brightnessSaved) {
                            DebugLog("Brightness down? 0x%x -> 0x%x", brightnessSaved, result);
                            dispatchKeyEvent(ADB_BRIGHTNESS_DOWN, true, false);
                            dispatchKeyEvent(ADB_BRIGHTNESS_DOWN, false, false);
                        } else {
                            DebugLog("Brightness up? 0x%x -> 0x%x", brightnessSaved, result);
                            dispatchKeyEvent(ADB_BRIGHTNESS_UP, true, false);
                            dispatchKeyEvent(ADB_BRIGHTNESS_UP, false, false);
                        }
                        brightnessSaved = result;
                        data = result;
                    }
                    time = 1;
                    break;

                case 5:
                    if (!read_ec_data(VPCCMD_R_TOUCHPAD, &result, &retries))
                        AlwaysLog("Failed to read VPCCMD_R_TOUCHPAD %d", retries);
                    else
                        DebugLog("Fn+F6 touchpad 0x%x %s", result, result ? "on" : "off");
                    if (TouchPadEnabledHW == (result != 0)) {
                        DebugLog("Skip duplicate touchpad report");
                        continue;
                    }
                    TouchPadEnabledHW = (result != 0);
                    data = result;
                    time = 1;
                    break;

                case 6: // KEY_SWITCHVIDEOMODE
                    DebugLog("Fn+F10 mirror");
                    time = 1;
                    break;

                case 7:
                    DebugLog("Fn+F8 camera");
                    time = 1;
                    break;

                case 8: // ENERGY_EVENT_MIC
                    DebugLog("Fn+F4 mic");
                    time = 1;
                    break;

                case 9: // RFKILL
                    DebugLog("Fn rfkill");
                    time = 1;
                    break;

                case 10:
                    DebugLog("Touchpad on");
                    time = 1;
                    break;

                case 11: // KEY_F16
                    DebugLog("Fn F16");
                    time = 1;
                    break;

                case 12: // ENERGY_EVENT_KEYBDLED
                    DebugLog("Fn+Space keyboard backlight?");
                    updateKeyboard(true);
                    data = backlightLevel;
                    time = 1;
                    break;

                case 13:
                    DebugLog("Fn+F7 airplane mode");
                    time = 1;
                    break;

                default:
                    AlwaysLog("Unknown VPC event %d", vpc_bit);
                    break;
            }

            if (client != nullptr)
                client->sendNotification(vpc_bit, data);
        }
    }

    if (time != 0) {
        clock_get_uptime(&time);
        dispatchMessage(kPS2M_notifyKeyTime, &time);
    }
}

// Write VCMD -> VCMD is cleared -> Read VDAT
bool IdeaVPC::read_ec_data(UInt32 cmd, UInt32 *result, UInt32 *retries) {
    if (!method_vpcw(1, cmd))
        return false;

    AbsoluteTime abstime, deadline, now_abs;
    nanoseconds_to_absolutetime(IDEAPAD_EC_TIMEOUT * 1000000ULL, &abstime);
    clock_get_uptime(&now_abs);

    for (clock_absolutetime_interval_to_deadline(abstime, &deadline);
         now_abs < deadline; ) {
        if (!method_vpcr(1, result))
            return false;
        if (*result == 0)
            return method_vpcr(0, result);
        *retries = *retries + 1;
        IODelay(250);
        clock_get_uptime(&now_abs);
    }

    AlwaysLog(timeoutPrompt, readECPrompt, cmd);
    return false;
}

// Write VDAT -> Write VCMD -> VDAT is cleared
bool IdeaVPC::write_ec_data(UInt32 cmd, UInt32 value, UInt32 *retries) {
    UInt32 result;

    if (!method_vpcw(0, value))
        return false;

    if (!method_vpcw(1, cmd))
        return false;

    AbsoluteTime abstime, deadline, now_abs;
    nanoseconds_to_absolutetime(IDEAPAD_EC_TIMEOUT * 1000000ULL, &abstime);
    clock_get_uptime(&now_abs);

    for (clock_absolutetime_interval_to_deadline(abstime, &deadline);
         now_abs < deadline; ) {
        if (!method_vpcr(1, &result))
            return false;
        if (result == 0)
            return true;
        *retries = *retries + 1;
        IODelay(250);
        clock_get_uptime(&now_abs);
    }

    AlwaysLog(timeoutPrompt, writeECPrompt, cmd);
    return false;
}

bool IdeaVPC::method_vpcr(UInt32 cmd, UInt32 *result) {
    OSObject* params[1] = {
        OSNumber::withNumber(cmd, 32)
    };

    IOReturn ret = vpc->evaluateInteger(readVPCStatus, result, params, 1);
    params[0]->release();

    return (ret == kIOReturnSuccess);
}

bool IdeaVPC::method_vpcw(UInt32 cmd, UInt32 data) {
    UInt32 result; // should only return 0

    OSObject* params[2] = {
        OSNumber::withNumber(cmd, 32),
        OSNumber::withNumber(data, 32)
    };

    IOReturn ret = vpc->evaluateInteger(writeVPCStatus, &result, params, 2);
    params[0]->release();
    params[1]->release();

    return (ret == kIOReturnSuccess);
}
