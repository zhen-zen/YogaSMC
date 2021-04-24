//  SPDX-License-Identifier: GPL-2.0-only
//
//  YogaVPC.cpp
//  YogaSMC
//
//  Created by Zhen on 2020/7/10.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "YogaVPC.hpp"

OSDefineMetaClassAndStructors(YogaVPC, YogaBaseService);

IOService *YogaVPC::probe(IOService *provider, SInt32 *score)
{
    if (!super::probe(provider, score))
        return nullptr;
 
    if (!probeVPC(provider))
        return nullptr;
 
    DebugLog("Probing");

    vpc = OSDynamicCast(IOACPIPlatformDevice, provider);
    if (!vpc)
        return nullptr;

    auto pnp = OSString::withCString(PnpDeviceIdEC);
    ec = OSDynamicCast(IOACPIPlatformDevice, vpc->getParentEntry(gIOACPIPlane));
    if (!(ec && ec->IORegistryEntry::compareName(pnp)))
        findPNP(PnpDeviceIdEC, &ec);
    pnp->release();

    if (!ec) {
        AlwaysLog("Failed to find EC");
        return nullptr;
    }

    auto iterator = IORegistryIterator::iterateOver(gIOACPIPlane, kIORegistryIterateRecursively);
    if (!iterator) {
        AlwaysLog("findWMI failed");
    } else {
        auto pnp = OSString::withCString(PnpDeviceIdWMI);
        IOACPIPlatformDevice *dev;
        WMICollection = OSOrderedSet::withCapacity(1);
        WMIProvCollection = OSOrderedSet::withCapacity(1);

        while (auto entry = iterator->getNextObject()) {
            if (entry->compareName(pnp) &&
                (dev = OSDynamicCast(IOACPIPlatformDevice, entry))) {
                OSObject *uid = nullptr;
                if (dev->evaluateObject("_UID", &uid) == kIOReturnSuccess) {
                    OSString *str = OSDynamicCast(OSString, uid);
                    if (str && str->isEqualTo("TBFP")) {
                        DebugLog("Skip Thunderbolt interface");
                        continue;
                    }
                }
                OSSafeReleaseNULL(uid);
                if (auto wmi = initWMI(dev)) {
                    DebugLog("WMI available at %s", dev->getName());
                    WMICollection->setObject(wmi);
                    WMIProvCollection->setObject(dev);
                    wmi->release();
                } else {
                    DebugLog("WMI init failed at %s", dev->getName());
                }
            }
        }
        if (WMICollection->getCount() == 0) {
            OSSafeReleaseNULL(WMICollection);
            OSSafeReleaseNULL(WMIProvCollection);
        }
        iterator->release();
        pnp->release();
    }

#ifndef ALTER
    if (getProperty("Sensors") != nullptr)
        initSMC();
#endif
    return this;
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
            if (!wmi->start(prov)) {
                AlwaysLog("Failed to start WMI instance on %s", prov->getName());
                wmi->detach(prov);
                WMICollection->removeObject(wmi);
                WMIProvCollection->removeObject(prov);
            }
        }
    }
#ifndef ALTER
    if (smc)
        smc->start(this);
#endif
    setProperty(kDeliverNotifications, kOSBooleanTrue);
    registerService();
    return true;
}

bool YogaVPC::initVPC() {
    if (vpc->validateObject(getClamshellMode) == kIOReturnSuccess &&
        vpc->validateObject(setClamshellMode) == kIOReturnSuccess)
        clamshellCap = true;

    if (vpc->validateObject(setThermalControl) == kIOReturnSuccess) {
        DYTC_RESULT result;
        if (DYTCCommand(dytc_query_cmd, &result)) {
            DYTCCap = result.query.enable;
            if (DYTCCap) {
                DYTCVersion = OSDictionary::withCapacity(2);
                OSObject *value;
                setPropertyNumber(DYTCVersion, "Revision", result.query.rev, 4);
                setPropertyNumber(DYTCVersion, "SubRevision", (result.query.subrev_hi << 8) + result.query.subrev_lo, 12);
                if (result.query.rev >= 5)
                    DYTCLapmodeCap = true;
            }
            DebugLog(updateSuccess, DYTCPrompt, DYTCCap);
        } else {
            setProperty("DYTC", "error");
            AlwaysLog(updateFailure, DYTCPrompt);
        }
    }

    return true;
}

bool YogaVPC::exitVPC() {
    if (clamshellMode) {
        AlwaysLog("Disabling clamshell mode");
        toggleClamshell();
    }
    return true;
}

void YogaVPC::stop(IOService *provider) {
    DebugLog("Stopping");
    exitVPC();

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

#ifndef ALTER
    if (smc) {
        smc->stop(this);
        smc->detach(this);
    }
#endif
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
            } else if (key->isEqualTo("ReadECOffset")) {
                if (!(ECAccessCap & BIT(0))) {
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
                if (!setDYTC(mode)) {
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

    OSObject* params[1] = {
        OSNumber::withNumber(!clamshellMode, 32)
    };

    IOReturn ret = vpc->evaluateInteger(setClamshellMode, &result, params, 1);
    params[0]->release();

    if (ret != kIOReturnSuccess || result != 0) {
        AlwaysLog(toggleFailure, clamshellPrompt);
        return false;
    }

    clamshellMode = !clamshellMode;
    DebugLog(toggleSuccess, clamshellPrompt, clamshellMode, (clamshellMode ? "on" : "off"));
    setProperty(clamshellPrompt, clamshellMode);

    return true;
}

bool YogaVPC::notifyBattery() {
    if (ec->validateObject("NBAT") != kIOReturnSuccess ||
        ec->evaluateObject("NBAT") != kIOReturnSuccess ) {
        DebugLog(toggleFailure, "NBAT");
        return false;
    }
    return true;
}

IOReturn YogaVPC::setPowerState(unsigned long powerStateOrdinal, IOService * whatDevice) {
    if (super::setPowerState(powerStateOrdinal, whatDevice) != kIOPMAckImplied)
        return kIOReturnInvalid;

    if (backlightCap && (automaticBacklightMode & BIT(0))) {
        updateBacklight();
        if (powerStateOrdinal == 0) {
            backlightLevelSaved = backlightLevel;
            if (backlightLevel)
                setBacklight(0);
        } else {
            if (!backlightLevel && backlightLevelSaved)
                setBacklight(backlightLevelSaved);
        }
    }

    return kIOPMAckImplied;
}

bool YogaVPC::DYTCCommand(DYTC_CMD command, DYTC_RESULT* result, UInt8 ICFunc, UInt8 ICMode, bool ValidF) {
    DYTCLock = true;
    if (command.raw == DYTC_CMD_SET) {
        command.ICFunc = ICFunc;
        command.ICMode = ICMode;
        command.validF = ValidF;
    }
    OSObject* params[1] = {
        OSNumber::withNumber(command.raw, 32)
    };

    IOReturn ret = vpc->evaluateInteger(setThermalControl, &(result->raw), params, 1);
    params[0]->release();

    if (ret != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, DYTCPrompt);
        DYTCCap = false;
        setProperty("DYTC", false);
    } else {
        switch (result->errorcode) {
            case DYTC_SUCCESS:
                DebugLog("%s command 0x%x result 0x%08x", DYTCPrompt, command.raw, result->raw);
                break;

            case DYTC_UNSUPPORTED:
            case DYTC_DPTF_UNAVAILABLE:
                AlwaysLog("%s command 0x%x result 0x%08x (unsuppported)", DYTCPrompt, command.raw, result->raw);
                DYTCCap = false;
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
    }
    DYTCLock = false;
    return (result->errorcode == DYTC_SUCCESS);
}

bool YogaVPC::parseDYTC(DYTC_RESULT result) {
    OSDictionary *status = OSDictionary::withDictionary(DYTCVersion);
    OSObject *value;

    switch (result.get.funcmode) {
        case DYTC_FUNCTION_STD:
            setPropertyString(status, "FuncMode", "Standard");
            break;

        case DYTC_FUNCTION_CQL:
            /* We can't get the mode when in CQL mode - so we disable CQL
             * mode retrieve the mode and then enable it again.
             */
            DYTC_RESULT dummy;
            if (!DYTCCommand(dytc_set_cmd, &dummy, DYTC_FUNCTION_CQL, 0xf, false) ||
                !DYTCCommand(dytc_get_cmd, &result) ||
                !DYTCCommand(dytc_set_cmd, &dummy, DYTC_FUNCTION_CQL, 0xf, true)) {
                status->release();
                return false;
            }
            setPropertyString(status, "FuncMode", "Lap");
            break;

        case DYTC_FUNCTION_MMC:
            setPropertyString(status, "FuncMode", "Desk");
            break;

        default:
            AlwaysLog(valueUnknown, DYTCFuncPrompt, result.get.funcmode);
            char Unknown[10];
            snprintf(Unknown, 10, "Unknown:%1x", result.get.funcmode);
            setPropertyString(status, "FuncMode", Unknown);
            break;
    }

    switch (result.get.perfmode) {
        case DYTC_MODE_PERFORM:
            if (result.get.funcmode == DYTC_FUNCTION_CQL)
                setPropertyString(status, "PerfMode", "Performance (Reduced as lapmode active)");
            else
                setPropertyString(status, "PerfMode", "Performance");
            break;

        case DYTC_MODE_QUIET:
            setPropertyString(status, "PerfMode", "Quiet");
            break;

        case DYTC_MODE_BALANCE:
            setPropertyString(status, "PerfMode", "Balance");
            break;

        default:
            AlwaysLog(valueUnknown, DYTCPerfPrompt, result.get.perfmode);
            char Unknown[10];
            snprintf(Unknown, 10, "Unknown:%1x", result.get.perfmode);
            setPropertyString(status, "PerfMode", Unknown);
            break;
    }

    for (int func_bit = 0; func_bit < 16; func_bit++) {
        if (BIT(func_bit) & result.get.vmode) {
            switch (func_bit) {
                case DYTC_FUNCTION_STD:
                    DebugLog("Found DYTC_FUNCTION_STD");
                    break;

                case DYTC_FUNCTION_CQL:
                    DebugLog("Found DYTC_FUNCTION_CQL");
                    break;

                case DYTC_FUNCTION_MMC:
                    DebugLog("Found DYTC_FUNCTION_MMC");
                    break;

                case DYTC_FUNCTION_STP:
                    DebugLog("Found DYTC_FUNCTION_STP");
                    break;

                default:
                    AlwaysLog("Unknown DYTC function %x", func_bit);
                    break;
            }
        }
    }

    if (DYTCLapmodeCap)
        setPropertyBoolean(status, "lapmode", BIT(DYTC_FUNCTION_CQL) & result.get.vmode);

    setProperty("DYTC", status);
    status->release();
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

bool YogaVPC::setDYTC(int perfmode) {
    if (!DYTCCap)
        return true;

    DYTC_RESULT result;
    if (perfmode == DYTC_MODE_BALANCE) {
        // Or set DYTC_FUNCTION_MMC / DYTC_MODE_BALANCE / false
        if (!DYTCCommand(dytc_reset_cmd, &result))
            return false;
    } else {
        if (!DYTCCommand(dytc_get_cmd, &result))
            return false;
        if (result.get.funcmode == DYTC_FUNCTION_CQL) {
            if (!DYTCCommand(dytc_set_cmd, &result, DYTC_FUNCTION_CQL, 0xf, false) ||
                !DYTCCommand(dytc_set_cmd, &result, DYTC_FUNCTION_MMC, perfmode, true) ||
                !DYTCCommand(dytc_set_cmd, &result, DYTC_FUNCTION_CQL, 0xf, true))
                return false;
        } else {
            if (!DYTCCommand(dytc_set_cmd, &result, DYTC_FUNCTION_MMC, perfmode, true))
                return false;
        }
    }
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

