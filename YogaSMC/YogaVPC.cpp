//  SPDX-License-Identifier: GPL-2.0-only
//
//  YogaVPC.cpp
//  YogaSMC
//
//  Created by Zhen on 2020/7/10.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "YogaVPC.hpp"
#ifndef ALTER
#include "YogaSMC.hpp"
#endif

OSDefineMetaClassAndStructors(YogaVPC, YogaBaseService);

IOService *YogaVPC::probe(IOService *provider, SInt32 *score)
{
    if (!super::probe(provider, score))
        return nullptr;

    if (!probeVPC(provider))
        return nullptr;

    DebugLog("Probing");

    if (!(vpc = OSDynamicCast(IOACPIPlatformDevice, provider)))
        return nullptr;

    if ((ec = OSDynamicCast(IOACPIPlatformDevice, vpc->getParentEntry(gIOACPIPlane)))) {
        DebugLog("Parent ACPI device: %s", ec->getName());
    }

    if (!findPNP(PnpDeviceIdEC, &ec)) {
        AlwaysLog("Failed to find EC");
        return nullptr;
    }

    isPMsupported = true;

    if (vendorWMISupport)
        probeWMI();

#ifndef ALTER
    if (getProperty("Sensors") != nullptr)
        smc = initSMC();
#endif
    return this;
}

void YogaVPC::probeWMI() {
    auto iterator = IORegistryIterator::iterateOver(gIOACPIPlane, kIORegistryIterateRecursively);
    if (!iterator) {
        AlwaysLog("findWMI failed");
        return;
    }

    auto pnp = OSString::withCString(PnpDeviceIdWMI);
    IOACPIPlatformDevice *dev;
    WMICollection = OSOrderedSet::withCapacity(1);
    WMIProvCollection = OSOrderedSet::withCapacity(1);

    while (auto entry = iterator->getNextObject()) {
        if (entry->compareName(pnp) && (dev = OSDynamicCast(IOACPIPlatformDevice, entry))) {
            if (dev == vpc) {
                DebugLog("Skip VPC interface");
                continue;
            }

            WMI *candidate = new WMI(dev);
            candidate->initialize();
            if (candidate->hasMethod(TBT_WMI_GUID)) {
                DebugLog("Skip Thunderbolt interface");
                delete candidate;
                continue;
            }

            if (auto wmi = initWMI(candidate)) {
                DebugLog("WMI available at %s", dev->getName());
                WMICollection->setObject(wmi);
                WMIProvCollection->setObject(dev);
                wmi->release();
            } else {
                DebugLog("WMI init failed at %s", dev->getName());
                delete candidate;
            }
        }
    }

    if (WMICollection->getCount() == 0) {
        OSSafeReleaseNULL(WMICollection);
        OSSafeReleaseNULL(WMIProvCollection);
    } else {
        setProperty("YogaWMISupported", true);
    }

    iterator->release();
    pnp->release();
}

bool YogaVPC::start(IOService *provider) {
    if (!super::start(provider))
        return false;
    DebugLog("Starting");

    if (!initVPC())
        return false;

    validateEC();

    updateAll();

    if (WMICollection) {
        for (int i=WMICollection->getCount()-1; i >= 0; i--) {
            IOService *wmi = OSDynamicCast(IOService, WMICollection->getObject(i));
            IOService *prov = OSDynamicCast(IOService, WMIProvCollection->getObject(i));
            if (!wmi->attach(prov)) {
                AlwaysLog("Failed to attach WMI instance on %s", prov->getName());
                WMICollection->removeObject(wmi);
                WMIProvCollection->removeObject(prov);
            } else if (!wmi->start(prov)) {
                wmi->detach(prov);
                AlwaysLog("Failed to start WMI instance on %s", prov->getName());
                WMICollection->removeObject(wmi);
                WMIProvCollection->removeObject(prov);
            } else {
                examineWMI(wmi);
            }
        }
    }
#ifndef ALTER
    if (smc) {
        if (!smc->attach(this)) {
            OSSafeReleaseNULL(smc);
            AlwaysLog("Failed to attach SMC instance");
        } else if (!smc->start(this)) {
            smc->detach(this);
            AlwaysLog("Failed to start SMC instance");
            OSSafeReleaseNULL(smc);
        }
    }
#endif
    setProperty(kDeliverNotifications, kOSBooleanTrue);
    registerService();
    return true;
}

bool YogaVPC::initVPC() {
    if (vpc->validateObject(getClamshellMode) == kIOReturnSuccess &&
        vpc->validateObject(setClamshellMode) == kIOReturnSuccess)
        clamshellCap = true;

    if (vpc->validateObject(setThermalControl) == kIOReturnSuccess)
        initDYTC();

    backlightPoller = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &YogaVPC::backlightRest));
    if (!backlightPoller ||
        workLoop->addEventSource(backlightPoller) != kIOReturnSuccess) {
        AlwaysLog("Failed to add brightnessPoller");
        OSSafeReleaseNULL(backlightPoller);
    } else {
        backlightPoller->enable();
        setProperty(backlightTimeoutPrompt, backlightTimeout, 32);
        DebugLog("BrightnessPoller enabled");
    }

    return true;
}

bool YogaVPC::exitVPC() {
    if (backlightPoller) {
        backlightPoller->disable();
        workLoop->removeEventSource(backlightPoller);
    }
    OSSafeReleaseNULL(backlightPoller);

    if (clamshellMode) {
        AlwaysLog("Disabling clamshell mode");
        toggleClamshell();
    }

    if (DYTCCap)
        OSSafeReleaseNULL(DYTCVersion);

    return true;
}

void YogaVPC::stop(IOService *provider) {
    DebugLog("Stopping");
    exitVPC();
#ifndef ALTER
    if (smc) {
        smc->stop(this);
        smc->detach(this);
    }
#endif
    if (WMICollection) {
        for (int i= WMICollection->getCount()-1; i >= 0; i--) {
            IOService *wmi = OSDynamicCast(IOService, WMICollection->getObject(i));
            IOService *prov = OSDynamicCast(IOService, WMIProvCollection->getObject(i));
            wmi->stop(prov);
            wmi->detach(prov);
        }
        OSSafeReleaseNULL(WMICollection);
        OSSafeReleaseNULL(WMIProvCollection);
    }

    super::stop(provider);
}

void YogaVPC::updateAll() {
    updateBacklight();
    updateClamshell();
    if (!updateDYTC())
        AlwaysLog("Update DYTC failed");
}

void YogaVPC::setPropertiesGated(OSObject* props) {
    OSDictionary* dict = OSDynamicCast(OSDictionary, props);
    if (!dict)
        return;

//    DebugLog("Found %d objects in properties", dict->getCount());
    OSCollectionIterator* i = OSCollectionIterator::withCollection(dict);

    if (i) {
        while (OSString* key = OSDynamicCast(OSString, i->getNextObject())) {
            if (key->isEqualTo(clamshellPrompt)) {
                if (!clamshellCap) {
                    AlwaysLog(notSupported, clamshellPrompt);
                    continue;
                }
                OSBoolean *value;
                getPropertyBoolean(clamshellPrompt);
                updateClamshell(false);
                if (value->getValue() == clamshellMode)
                    DebugLog(valueMatched, clamshellPrompt, clamshellMode);
                else
                    toggleClamshell();
            } else if (key->isEqualTo(backlightPrompt)) {
                OSNumber *value;
                getPropertyNumber(backlightPrompt);
                if (!updateBacklight()) {
                    AlwaysLog(notSupported, backlightPrompt);
                    continue;
                }
                if (value->unsigned32BitValue() == backlightLevel)
                    DebugLog(valueMatched, backlightPrompt, backlightLevel);
                else
                    setBacklight(value->unsigned32BitValue());
            } else if (key->isEqualTo(autoBacklightPrompt)) {
                OSNumber *value;
                getPropertyNumber(autoBacklightPrompt);
                if (value->unsigned8BitValue() > 0x1f) {
                    AlwaysLog(valueInvalid, autoBacklightPrompt);
                } else if (value->unsigned8BitValue() == automaticBacklightMode) {
                    DebugLog(valueMatched, autoBacklightPrompt, automaticBacklightMode);
                } else {
                    automaticBacklightMode = value->unsigned8BitValue();
                    setProperty(autoBacklightPrompt, automaticBacklightMode, 8);
                }
            } else if (key->isEqualTo(backlightTimeoutPrompt)) {
                OSNumber *value;
                getPropertyNumber(backlightTimeoutPrompt);
                backlightTimeout = value->unsigned32BitValue();
                backlightLevelSaved = 1;
                setProperty(backlightTimeoutPrompt, backlightTimeout, 32);
                if (backlightTimeout == 0)
                    backlightPoller->cancelTimeout();
            } else if (key->isEqualTo(FnKeyPrompt)) {
                OSBoolean *value;
                getPropertyBoolean(FnKeyPrompt);
                updateFnLock();

                if (value->getValue() == FnlockMode)
                    DebugLog(valueMatched, FnKeyPrompt, FnlockMode);
                else
                    setFnLock(!FnlockMode);
            } else if (key->isEqualTo("ReadECOffset")) {
                if (!(ECAccessCap & ECReadCap)) {
                    AlwaysLog(notSupported, "EC Read");
                    continue;
                }
                OSNumber *value;
                getPropertyNumber("ReadECOffset");
                dumpECOffset(value->unsigned32BitValue());
            } else if (key->isEqualTo("ReadECName")) {
                OSString *value;
                getPropertyString("ReadECName");
                if (value->getLength() !=4) {
                    AlwaysLog("invalid length %d", value->getLength());
                    continue;
                }
                UInt32 result;
                if (readECName(value->getCStringNoCopy(), &result) == kIOReturnSuccess)
                    AlwaysLog("%s: 0x%02x", value->getCStringNoCopy(), result);
            } else if (key->isEqualTo(DYTCPSCPrompt)) {
                if (!(DYTCCap & BIT(DYTC_FUNCTION_PSC))) {
                    AlwaysLog(notSupported, DYTCPSCPrompt);
                    continue;
                }
                OSNumber *value;
                getPropertyNumber(DYTCPSCPrompt);
                if (!setDYTC(DYTC_FUNCTION_PSC, value->unsigned8BitValue())) {
                    AlwaysLog(toggleFailure, DYTCPSCPrompt);
                    updateDYTC();
                } else {
                    DebugLog(toggleSuccess, DYTCPSCPrompt, value->unsigned8BitValue(), "see ioreg");
                }
            } else if (key->isEqualTo(DYTCPrompt)) {
                if (!DYTCCap) {
                    AlwaysLog(notSupported, DYTCPrompt);
                    continue;
                }
                OSNumber *raw = OSDynamicCast(OSNumber, dict->getObject(DYTCPrompt));
                if (raw != nullptr) {
                    DYTC_CMD cmd = {.raw = raw->unsigned32BitValue()};
                    DYTC_RESULT res;
                    if (DYTCCommand(cmd, &res) && parseDYTC(res))
                        AlwaysLog(toggleSuccess, DYTCPrompt, raw->unsigned32BitValue(), "see ioreg");
                    else
                        AlwaysLog(toggleFailure, DYTCPrompt);
                    continue;
                }
                OSString *value;
                getPropertyString(DYTCPrompt);
                int mode;
                switch (value->getChar(0)) {
                    case 'l':
                    case 'L':
                    case 'q':
                    case 'Q':
                        mode = DYTC_MODE_QUIET;
                        break;

                    case 'b':
                    case 'B':
                    case 'm':
                    case 'M':
                        mode = DYTC_MODE_BALANCE;
                        break;

                    case 'h':
                    case 'H':
                    case 'p':
                    case 'P':
                        mode = DYTC_MODE_PERFORM;
                        break;

                    default:
                        AlwaysLog(valueInvalid, DYTCPrompt);
                        continue;
                }
                if (!setDYTC(DYTC_FUNCTION_MMC, mode)) {
                    AlwaysLog(toggleFailure, DYTCPrompt);
                    updateDYTC();
                } else {
                    DebugLog(toggleSuccess, DYTCPrompt, mode, "see ioreg");
                }
            } else if (key->isEqualTo(updatePrompt)) {
                updateAll();
            } else {
                AlwaysLog("Unknown property %s", key->getCStringNoCopy());
            }
        }
        i->release();
    }

    return;
}

IOReturn YogaVPC::setProperties(OSObject *props) {
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &YogaVPC::setPropertiesGated), props);
    return kIOReturnSuccess;
}

bool YogaVPC::updateClamshell(bool update) {
    if (!clamshellCap)
        return true;

    UInt32 state;
    if (vpc->evaluateInteger(getClamshellMode, &state) != kIOReturnSuccess) {
        AlwaysLog(updateFailure, clamshellPrompt);
        return false;
    }

    clamshellMode = state ? true : false;

    if (update) {
        DebugLog(updateSuccess, clamshellPrompt, state);
        setProperty(clamshellPrompt, clamshellMode);
    }

    return true;
}

bool YogaVPC::toggleClamshell() {
    if (!clamshellCap)
        return true;

    UInt32 result;
    if (evaluateIntegerParam(setClamshellMode, &result, !clamshellMode) != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, clamshellPrompt);
        return false;
    }

    clamshellMode = !clamshellMode;
    DebugLog(toggleSuccess, clamshellPrompt, clamshellMode, (clamshellMode ? "on" : "off"));
    setProperty(clamshellPrompt, clamshellMode);

    return true;
}

bool YogaVPC::notifyBattery() {
    if (ec->validateObject("NBAT") == kIOReturnSuccess &&
        ec->evaluateObject("NBAT") != kIOReturnSuccess ) {
        DebugLog(toggleFailure, "NBAT");
        return false;
    }
    return true;
}

IOReturn YogaVPC::setPowerState(unsigned long powerStateOrdinal, IOService * whatDevice) {
    if (super::setPowerState(powerStateOrdinal, whatDevice) != kIOPMAckImplied)
        return kIOReturnInvalid;

    if (!backlightCap)
        return kIOPMAckImplied;

    if (automaticBacklightMode & BIT(0) || backlightTimeout != 0) {
        updateBacklight();
        if (powerStateOrdinal == 0) {
            if (backlightTimeout != 0)
                backlightPoller->cancelTimeout();
            else
                backlightLevelSaved = backlightLevel;
            if (backlightLevel)
                setBacklight(0);
        } else {
            if (!backlightLevel && backlightLevelSaved)
                setBacklight(backlightLevelSaved);
            if (backlightTimeout != 0)
                backlightPoller->setTimeout(backlightTimeout, kSecondScale);
        }
    }

    return kIOPMAckImplied;
}

void YogaVPC::backlightRest(OSObject *owner, IOTimerEventSource *timer) {
    DebugLog("backlight %x saved %x", backlightLevel, backlightLevelSaved);
    backlightLevelSaved = backlightLevel;
    clock_get_uptime(&backlightStimulation);
    setBacklight(0);
}

bool YogaVPC::DYTCCommand(DYTC_CMD command, DYTC_RESULT* result, UInt8 ICFunc, UInt8 ICMode, bool ValidF) {
    if (DYTCLock)
        return false;

    DYTCLock = true;
    if (command.raw == DYTC_CMD_SET) {
        command.ICFunc = ICFunc;
        command.ICMode = ICMode;
        command.validF = ValidF;
    }

    if (evaluateIntegerParam(setThermalControl, &(result->raw), command.raw) != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, DYTCPrompt);
        DYTCLock = false;
        return false;
    }

    switch (result->errorcode) {
        case DYTC_SUCCESS:
            DebugLog("%s command 0x%x result 0x%08x", DYTCPrompt, command.raw, result->raw);
            break;

        case DYTC_UNSUPPORTED:
        case DYTC_DPTF_UNAVAILABLE:
            AlwaysLog("%s command 0x%x result 0x%08x (unsuppported)", DYTCPrompt, command.raw, result->raw);
            DYTCCap = 0;
            setProperty("DYTC", false);
            break;

        case DYTC_FUNC_INVALID:
        case DYTC_CMD_INVALID:
        case DYTC_MODE_INVALID:
            AlwaysLog("%s command 0x%x result 0x%08x (invalid argument)", DYTCPrompt, command.raw, result->raw);
            break;

        case DYTC_EXCEPTION:
        default:
            AlwaysLog("%s command 0x%x result 0x%08x error %d", DYTCPrompt, command.raw, result->raw, result->errorcode);
            break;
    }

    DYTCLock = false;
    return (result->errorcode == DYTC_SUCCESS);
}

void YogaVPC::initDYTC() {
    DYTC_RESULT result;
    if (!DYTCCommand(dytc_query_cmd, &result)) {
        setProperty("DYTC", "error");
        AlwaysLog(updateFailure, DYTCPrompt);
        return;
    }

    if (!result.query.enable) {
        AlwaysLog("%s not enabled", DYTCPrompt);
        return;
    }

    // FCAP (16-bit EC field) read might be broken
    DYTCCap = BIT(DYTC_FUNCTION_STD) | BIT(DYTC_FUNCTION_CQL) | BIT(DYTC_FUNCTION_MMC);

    DYTCVersion = OSDictionary::withCapacity(3);
    OSObject *value;
    setPropertyNumber(DYTCVersion, "Revision", result.query.rev, 4);
    setPropertyNumber(DYTCVersion, "SubRevision", (result.query.subrev_hi << 8) + result.query.subrev_lo, 12);

    if (DYTCCommand(dytc_func_cap_cmd, &result)) {
        DYTCCap = result.func_cap.bitmap;
#ifdef DEBUG
        setPropertyNumber(DYTCVersion, "FCAP", result.func_cap.bitmap, 16);
#endif
    }

    setProperty("DYTC", DYTCVersion);
}

bool YogaVPC::parseDYTC(DYTC_RESULT result) {
    OSDictionary *DYTCStatus = OSDictionary::withDictionary(DYTCVersion);
    OSObject *value;

    bool lapmode = false;

    OSDictionary *functionStatus = OSDictionary::withCapacity(16);
    for (int func_bit = 0; func_bit < 16; func_bit++) {
        if (BIT(func_bit) & DYTCCap) {
            bool func_status = (BIT(func_bit) & result.get.func_sta);
            switch (func_bit) {
                case DYTC_FUNCTION_STD:
                    setPropertyBoolean(functionStatus, "STD", func_status);
                    break;

                case DYTC_FUNCTION_CQL:
                    if (func_status)
                        DebugLog("DYTC_FUNCTION_CQL on");
                    setPropertyBoolean(functionStatus, "CQL", func_status);
                    setPropertyBoolean(DYTCStatus, "lapmode", func_status);
                    lapmode = func_status;
                    break;

                case DYTC_FUNCTION_MYH:
                    if (func_status)
                        DebugLog("DYTC_FUNCTION_MYH on");
                    setPropertyBoolean(functionStatus, "MYH", func_status);
                    break;

                case DYTC_FUNCTION_STP:
                    if (func_status)
                        DebugLog("DYTC_FUNCTION_STP on");
                    setPropertyBoolean(functionStatus, "STP", func_status);
                    break;

                case DYTC_FUNCTION_MMC:
                    if (func_status)
                        DebugLog("DYTC_FUNCTION_MMC on");
                    setPropertyBoolean(functionStatus, "MMC", func_status);
                    break;

                case DYTC_FUNCTION_MSC:
                    if (func_status)
                        DebugLog("DYTC_FUNCTION_MSC on");
                    setPropertyBoolean(functionStatus, "MSC", func_status);
                    break;

                case DYTC_FUNCTION_PSC:
                    if (func_status)
                        DebugLog("DYTC_FUNCTION_PSC on");
                    setPropertyBoolean(functionStatus, "PSC", func_status);
                    break;

                default:
                    if (func_status)
                        AlwaysLog("Unknown DYTC Function 0x%X on", func_bit);
                    break;
            }
        }
    }
#ifdef DEBUG
    setPropertyNumber(functionStatus, "raw", result.get.func_sta, 16);
#endif
    DYTCStatus->setObject("Function Status", functionStatus);
    functionStatus->release();

    if (lapmode && !DYTCCommand(dytc_set_cmd, &result, DYTC_FUNCTION_CQL, 0xf, false)) {
        DYTCStatus->release();
        return false;
    }

    switch (result.get.funcmode) {
        case DYTC_FUNCTION_STD:
            setPropertyString(DYTCStatus, "FuncMode", "Standard");
            setPropertyString(DYTCStatus, "PerfMode", "Balance");
            break;

        case DYTC_FUNCTION_MMC:
            setPropertyString(DYTCStatus, "FuncMode", "Desk");
            switch (result.get.perfmode) {
                case DYTC_MODE_PERFORM:
                    setPropertyString(DYTCStatus, "PerfMode", lapmode ? "Performance (Reduced as lapmode active)" : "Performance");
                    break;

                case DYTC_MODE_QUIET:
                    setPropertyString(DYTCStatus, "PerfMode", "Quiet");
                    break;

                default:
                    AlwaysLog(valueUnknown, DYTCPerfPrompt, result.get.perfmode);
                    char Unknown[16];
                    snprintf(Unknown, 16, "UnknownPerf:0x%x", result.get.perfmode);
                    setPropertyString(DYTCStatus, "PerfMode", Unknown);
                    break;
            }
            break;

        case DYTC_FUNCTION_PSC:
            setPropertyString(DYTCStatus, "FuncMode", "Controlled");
            setPropertyNumber(DYTCStatus, "PerfMode", result.get.perfmode, 16);
            break;

        default:
            AlwaysLog(valueUnknown, DYTCFuncPrompt, result.get.funcmode);
            AlwaysLog(valueUnknown, DYTCPerfPrompt, result.get.perfmode);
            char Unknown[16];
            snprintf(Unknown, 16, "UnknownFunc:0x%x", result.get.funcmode);
            setPropertyString(DYTCStatus, "FuncMode", Unknown);
            snprintf(Unknown, 16, "UnknownPerf:0x%x", result.get.perfmode);
            setPropertyString(DYTCStatus, "PerfMode", Unknown);
            break;
    }

    if (lapmode) {
        setPropertyString(DYTCStatus, "FuncMode", "Lap");
        if (!DYTCCommand(dytc_set_cmd, &result, DYTC_FUNCTION_CQL, 0xf, true)) {
            DYTCStatus->release();
            return false;
        }
    }

    setProperty("DYTC", DYTCStatus);
    DYTCStatus->release();
    return true;
}

bool YogaVPC::updateDYTC() {
    if (!DYTCCap)
        return true;

    DYTC_RESULT result;
    if (!DYTCCommand(dytc_get_cmd, &result))
        return false;

    return parseDYTC(result);
}

bool YogaVPC::setDYTC(int funcmode, int perfmode) {
    if (!DYTCCap)
        return true;

    DYTC_RESULT result;

    if (!DYTCCommand(dytc_get_cmd, &result))
        return false;

    bool lapmode = (result.get.funcmode == DYTC_FUNCTION_CQL);

    if (!DYTCCommand(dytc_reset_cmd, &result))
            return false;

    if (perfmode != DYTC_MODE_BALANCE)
        if (!DYTCCommand(dytc_set_cmd, &result, funcmode, perfmode, true))
            return false;

    if (lapmode)
        if (!DYTCCommand(dytc_set_cmd, &result, DYTC_FUNCTION_CQL, 0xf, true))
            return false;

    return parseDYTC(result);
}

bool YogaVPC::dumpECOffset(UInt32 value) {
    bool ret = false;
    UInt32 size = value >> 8;
    if (size) {
        UInt32 offset = value & 0xff;
        if (offset + size > 0x100) {
            AlwaysLog("read %d bytes @ 0x%02x exceeded", size, offset);
        } else {
            OSData* result;
            if (method_recb(offset, size, &result) == kIOReturnSuccess) {
                const UInt8* data = reinterpret_cast<const UInt8 *>(result->getBytesNoCopy());
                int len = 0x10;
                char *buf = new char[len*3];
                if (size > 8) {
                    AlwaysLog("%d bytes @ 0x%02x", size, offset);
                    for (int i = 0; i < size; i += len) {
                        memset(buf, 0, len*3);
                        for (int j = 0; j < (len < size-i ? len : size-i); j++)
                            snprintf(buf+3*j, 4, "%02x ", data[i+j]);
                        buf[len*3-1] = '\0';
                        AlwaysLog("0x%02x: %s", offset+i, buf);
                    }
                } else {
                    for (int j = 0; j < size; j++)
                        snprintf(buf+3*j, 4, "%02x ", data[j]);
                    buf[size*3-1] = '\0';
                    AlwaysLog("%d bytes @ 0x%02x: %s", size, offset, buf);
                }
                delete [] buf;
                result->release();
                ret = true;
            }
        }
    } else {
        UInt8 byte;
        if (method_re1b(value, &byte) == kIOReturnSuccess) {
            AlwaysLog("0x%02x: %02x", value, byte);
            ret = true;
        }
    }
    return ret;
}

#ifndef ALTER
IOService* YogaVPC::initSMC() {
    return YogaSMC::withDevice(this, ec);
};
#endif

IOReturn YogaVPC::message(UInt32 type, IOService *provider, void *argument) {
    switch (type)
    {
        case kSMC_setDisableTouchpad:
        case kSMC_getDisableTouchpad:
        case kPS2M_notifyKeyTime:
        case kPS2M_resetTouchpad:
        case kSMC_setKeyboardStatus:
        case kSMC_getKeyboardStatus:
        case kSMC_notifyKeystroke:
            break;

        case kPS2M_notifyKeyPressed:
            if (backlightTimeout != 0 && backlightLevelSaved != 0) {
                backlightPoller->cancelTimeout();
                if (backlightLevel == 0) {
                    clock_get_uptime(&backlightStimulation);
                    setBacklight(backlightLevelSaved);
                }
                backlightPoller->setTimeout(backlightTimeout, kSecondScale);
            }
            break;

        case kIOACPIMessageDeviceNotification:
            if (!argument)
                AlwaysLog("message: Unknown ACPI notification");
            else
                AlwaysLog("message: Unknown ACPI notification 0x%04x", *(reinterpret_cast<UInt32 *>(argument)));
            break;

        default:
            if (argument)
                AlwaysLog("message: type=%x, provider=%s, argument=0x%04x", type, provider->getName(), *(reinterpret_cast<UInt32 *>(argument)));
            else
                AlwaysLog("message: type=%x, provider=%s", type, provider->getName());
    }

    return kIOReturnSuccess;
}

IOReturn YogaVPC::evaluateIntegerParam(const char *objectName, UInt32 *resultInt32, UInt32 paramInt32) {
    IOReturn ret;
    OSObject* params[] = { OSNumber::withNumber(paramInt32, 32) };
    if (resultInt32 != nullptr) {
        ret = vpc->evaluateInteger(objectName, resultInt32, params, 1);
    } else {
        ret = vpc->evaluateObject(objectName, nullptr, params, 1);
    }
    params[0]->release();
    return ret;
}
