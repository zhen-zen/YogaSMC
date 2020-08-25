//  SPDX-License-Identifier: GPL-2.0-only
//
//  YogaVPC.cpp
//  YogaSMC
//
//  Created by Zhen on 2020/7/10.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "YogaVPC.hpp"

OSDefineMetaClassAndStructors(YogaVPC, IOService);

bool YogaVPC::init(OSDictionary *dictionary)
{
    if (!super::init(dictionary))
        return false;;
    name = "";
    DebugLog("Initializing\n");

    extern kmod_info_t kmod_info;
    setProperty("YogaSMC,Build", __DATE__);
    setProperty("YogaSMC,Version", kmod_info.version);

    return true;
}

IOService *YogaVPC::probe(IOService *provider, SInt32 *score)
{
    name = provider->getName();
    DebugLog("Probing\n");

    if (!(vpc = OSDynamicCast(IOACPIPlatformDevice, provider)))
        return nullptr;

    IORegistryEntry* parent = vpc->getParentEntry(gIOACPIPlane);
    auto pnp = OSString::withCString(PnpDeviceIdEC);
    if (parent->compareName(pnp))
        ec = OSDynamicCast(IOACPIPlatformDevice, parent);
    pnp->release();

    if (!ec)
        return nullptr;
#ifndef ALTER
    initSMC();
#endif
    return super::probe(provider, score);
}

bool YogaVPC::start(IOService *provider) {
    if (!super::start(provider))
        return false;
    DebugLog("Starting\n");

    workLoop = IOWorkLoop::workLoop();
    commandGate = IOCommandGate::commandGate(this);
    if (!workLoop || !commandGate || (workLoop->addEventSource(commandGate) != kIOReturnSuccess)) {
        AlwaysLog("Failed to add commandGate\n");
        return false;
    }

    if (!initVPC())
        return false;

    updateAll();
    smc->start(this);

    PMinit();
    provider->joinPMtree(this);
    registerPowerDriver(this, IOPMPowerStates, kIOPMNumberPowerStates);

    setProperty(kDeliverNotifications, kOSBooleanTrue);
    registerService();
    return true;
}

bool YogaVPC::initVPC() {
    if (vpc->validateObject(getClamshellMode) == kIOReturnSuccess &&
        vpc->validateObject(setClamshellMode) == kIOReturnSuccess)
        clamshellCap = true;

    if (ec->validateObject(readECOneByte) == kIOReturnSuccess &&
        ec->validateObject(readECBytes) == kIOReturnSuccess)
        ECReadCap = true;

    if (vpc->validateObject(setThermalControl) == kIOReturnSuccess) {
        UInt64 result;
        if (DYTCCommand(DYTC_CMD_QUERY, &result)) {
            DYTCCap = (result >> DYTC_QUERY_ENABLE_BIT) & 0x1;
            if (DYTCCap) {
                DYTCRevision = (result >> DYTC_QUERY_REV_BIT) & 0xF;
                DYTCSubRevision = (result >> DYTC_QUERY_SUBREV_BIT) & 0xF;
                OSDictionary *status = OSDictionary::withCapacity(4);
                OSObject *value;
                setPropertyNumber(status, "Revision", DYTCRevision, 4);
                setPropertyNumber(status, "SubRevision", DYTCSubRevision, 4);
                setProperty("DYTC", status);
                status->release();
            } else {
                setProperty("DYTC", false);
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
        AlwaysLog("Disabling clamshell mode\n");
        toggleClamshell();
    }
    return true;
}

void YogaVPC::stop(IOService *provider) {
    DebugLog("Stopping\n");
    exitVPC();
#ifndef ALTER
    if (smc) {
        smc->stop(this);
        smc->detach(this);
    }
#endif
    workLoop->removeEventSource(commandGate);
    OSSafeReleaseNULL(commandGate);
    OSSafeReleaseNULL(workLoop);

    PMstop();

    terminate();
    detach(provider);
    super::stop(provider);
}

void YogaVPC::updateAll() {
    updateBacklight();
    updateClamshell();
    updateDYTC();
}

void YogaVPC::setPropertiesGated(OSObject* props) {
    OSDictionary* dict = OSDynamicCast(OSDictionary, props);
    if (!dict)
        return;

//    DebugLog("Found %d objects in properties\n", dict->getCount());
    OSCollectionIterator* i = OSCollectionIterator::withCollection(dict);

    if (i) {
        while (OSString* key = OSDynamicCast(OSString, i->getNextObject())) {
            if (key->isEqualTo(clamshellPrompt)) {
                OSBoolean * value;
                getPropertyBoolean(clamshellPrompt);
                updateClamshell(false);
                if (value->getValue() == clamshellMode) {
                    DebugLog(valueMatched, clamshellPrompt, clamshellMode);
                } else {
                    toggleClamshell();
                }
            } else if (key->isEqualTo(backlightPrompt)) {
                OSNumber * value;
                getPropertyNumber(backlightPrompt);
                updateBacklight(false);
                if (value->unsigned32BitValue() == backlightLevel)
                    DebugLog(valueMatched, backlightPrompt, backlightLevel);
                else
                    setBacklight(value->unsigned32BitValue());
            } else if (key->isEqualTo(autoBacklightPrompt)) {
                OSNumber * value;
                getPropertyNumber(autoBacklightPrompt);
                if (value->unsigned8BitValue() > 3) {
                    AlwaysLog(valueInvalid, autoBacklightPrompt);
                } else if (value->unsigned8BitValue() == automaticBacklightMode) {
                    DebugLog(valueMatched, autoBacklightPrompt, automaticBacklightMode);
                } else {
                    automaticBacklightMode = value->unsigned8BitValue();
                    setProperty(autoBacklightPrompt, automaticBacklightMode, 8);
                }
            } else if (key->isEqualTo("ReadECOffset")) {
                if (!ECReadCap) {
                    AlwaysLog("%s not supported\n", "EC Read");
                    continue;
                }
                OSNumber * value;
                getPropertyNumber("ReadECOffset");
                dumpECOffset(value->unsigned32BitValue());
            } else if (key->isEqualTo("ReadECName")) {
                OSString *value;
                getPropertyString("ReadECName");
                if (value->getLength() !=4) {
                    AlwaysLog("invalid length %d\n", value->getLength());
                    continue;
                }
                UInt32 result;
                if (readECName(value->getCStringNoCopy(), &result) == kIOReturnSuccess)
                    AlwaysLog("%s: 0x%02x\n", value->getCStringNoCopy(), result);
            } else if (key->isEqualTo(DYTCPrompt)) {
                if (!DYTCCap) {
                    AlwaysLog("%s not supported\n", DYTCPrompt);
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
                        continue;;
                }
                if (!setDYTC(mode))
                    AlwaysLog(toggleFailure, DYTCPrompt);
                else
                    DebugLog(toggleSuccess, DYTCPrompt, mode, "see ioreg");
            } else if (key->isEqualTo(updatePrompt)) {
                updateAll();
            } else {
                AlwaysLog("Unknown property %s\n", key->getCStringNoCopy());
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

IOReturn YogaVPC::message(UInt32 type, IOService *provider, void *argument) {
    if (argument) {
        AlwaysLog("message: type=%x, provider=%s, argument=0x%04x\n", type, provider->getName(), *((UInt32 *) argument));
        updateAll();
    } else {
        AlwaysLog("message: type=%x, provider=%s\n", type, provider->getName());
    }
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

    if (vpc->evaluateInteger(setClamshellMode, &result, params, 1) != kIOReturnSuccess || result != 0) {
        AlwaysLog(toggleFailure, clamshellPrompt);
        return false;
    }

    clamshellMode = !clamshellMode;
    DebugLog(toggleSuccess, clamshellPrompt, clamshellMode, (clamshellMode ? "on" : "off"));
    setProperty(clamshellPrompt, clamshellMode);

    return true;
}

IOReturn YogaVPC::setPowerState(unsigned long powerState, IOService *whatDevice){
    AlwaysLog("powerState %ld : %s\n", powerState, powerState ? "on" : "off");

    if (whatDevice != this)
        return kIOReturnInvalid;

    if (backlightCap && (automaticBacklightMode & BIT(0))) {
        updateBacklight();
        if (powerState == 0) {
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

bool YogaVPC::DYTCCommand(UInt32 command, UInt64* result, UInt8 ICFunc, UInt8 ICMode, bool ValidF) {
    UInt64 cmd = command & 0x1ff;
    cmd |= (ICFunc & 0xF) << DYTC_SET_FUNCTION_BIT;
    cmd |= (ICMode & 0xF) << DYTC_SET_MODE_BIT;
    cmd |= ValidF << DYTC_SET_VALID_BIT;

    OSObject* params[1] = {
        OSNumber::withNumber(cmd, 64)
    };

    if (vpc->evaluateInteger(setThermalControl, result, params, 1) != kIOReturnSuccess) {
        AlwaysLog(toggleFailure, DYTCPrompt);
        return false;
    }
#ifdef DEBUG
    AlwaysLog("%s 0x%llx 0x%llx\n", DYTCPrompt, cmd, *result);
#endif
    return true;
}

bool YogaVPC::updateDYTC(bool update) {
    if (!DYTCCap)
        return true;

    UInt64 result;
    if (!DYTCCommand(DYTC_CMD_GET, &result))
        return false;

    DYTCMode = result;
//    setProperty(DYTCPrompt, result, 64);
    OSDictionary *status = OSDictionary::withCapacity(4);
    OSObject *value;
    setPropertyNumber(status, "Revision", DYTCRevision, 4);
    setPropertyNumber(status, "SubRevision", DYTCSubRevision, 4);

    int funcmode = (result >> DYTC_GET_FUNCTION_BIT) & 0xF;
    switch (funcmode) {
        case DYTC_FUNCTION_STD:
            setPropertyString(status, "FuncMode", "Standard");
            break;

        case DYTC_FUNCTION_CQL:
           /* We can't get the mode when in CQL mode - so we disable CQL
            * mode retrieve the mode and then enable it again.
            */
            UInt64 dummy;
            if (!DYTCCommand(DYTC_CMD_SET, &dummy, DYTC_FUNCTION_CQL, 0xf, false) ||
                !DYTCCommand(DYTC_CMD_GET, &result) ||
                !DYTCCommand(DYTC_CMD_SET, &dummy, DYTC_FUNCTION_CQL, 0xf, true)) {
                status->release();
                return false;
            }
            setPropertyString(status, "FuncMode", "Lap");
            break;

        case DYTC_FUNCTION_MMC:
            setPropertyString(status, "FuncMode", "Desk");
            break;

        default:
            AlwaysLog(valueUnknown, DYTCFuncPrompt, funcmode);
            char Unknown[10];
            snprintf(Unknown, 10, "Unknown:%1x", funcmode);
            setPropertyString(status, "FuncMode", Unknown);
            break;
    }

    int perfmode = (result >> DYTC_GET_MODE_BIT) & 0xF;
    switch (perfmode) {
        case DYTC_MODE_PERFORM:
            if (funcmode == DYTC_FUNCTION_CQL)
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
            AlwaysLog(valueUnknown, DYTCPerfPrompt, perfmode);
            char Unknown[10];
            snprintf(Unknown, 10, "Unknown:%1x", perfmode);
            setPropertyString(status, "PerfMode", Unknown);
            break;
    }
    setProperty("DYTC", status);
    status->release();

    if (update)
        AlwaysLog("%s 0x%llx\n", DYTCPrompt, result);
    return true;
}

bool YogaVPC::setDYTC(int perfmode) {
    if (!DYTCCap)
        return true;

    UInt64 result;
    if (perfmode == DYTC_MODE_BALANCE) {
        if (!DYTCCommand(DYTC_CMD_RESET, &result))
            return false;
    } else {
        updateDYTC(false);
        bool cql = (((DYTCMode >> DYTC_GET_FUNCTION_BIT) & 0xF) == DYTC_FUNCTION_CQL);
        if (cql) {
            if (!DYTCCommand(DYTC_CMD_SET, &result, DYTC_FUNCTION_CQL, 0xf, false) ||
                !DYTCCommand(DYTC_CMD_SET, &result, perfmode, DYTC_FUNCTION_MMC, false) ||
                !DYTCCommand(DYTC_CMD_SET, &result, DYTC_FUNCTION_CQL, 0xf, true))
                return false;
        } else {
            if (!DYTCCommand(DYTC_CMD_SET, &result, perfmode, DYTC_FUNCTION_MMC, false))
                return false;
        }
    }
    return updateDYTC(true);
}

bool YogaVPC::findPNP(const char *id, IOACPIPlatformDevice **dev) {
    auto iterator = IORegistryIterator::iterateOver(gIOACPIPlane, kIORegistryIterateRecursively);
    if (!iterator) {
        AlwaysLog("findPNP failed\n");
        return nullptr;
    }
    auto pnp = OSString::withCString(id);

    while (auto entry = iterator->getNextObject()) {
        if (entry->compareName(pnp)) {
            *dev = OSDynamicCast(IOACPIPlatformDevice, entry);
            if (*dev) {
                AlwaysLog("%s available at %s\n", id, (*dev)->getName());
                break;
            }
        }
    }
    iterator->release();
    pnp->release();

    return !!(*dev);
}

IOReturn YogaVPC::readECName(const char* name, UInt32 *result) {
    IOReturn ret = ec->evaluateInteger(name, result);
    switch (ret) {
        case kIOReturnSuccess:
            break;

        case kIOReturnBadArgument:
            AlwaysLog("read %s failed, bad argument (field size too large?)\n", name);
            break;
            
        default:
            AlwaysLog("read %s failed %x\n", name, ret);
            break;
    }
    return ret;
}

IOReturn YogaVPC::method_re1b(UInt32 offset, UInt32 *result) {
    OSObject* params[1] = {
        OSNumber::withNumber(offset, 32)
    };

    IOReturn ret = ec->evaluateInteger(readECOneByte, result, params, 1);
    if (ret != kIOReturnSuccess)
        AlwaysLog("read 0x%02x failed\n", offset);

    return ret;
}

IOReturn YogaVPC::method_recb(UInt32 offset, UInt32 size, OSData **data) {
    if (!ECReadCap)
        return kIOReturnUnsupported;

    // Arg0 - offset in bytes from zero-based EC
    // Arg1 - size of buffer in bits
    OSObject* params[2] = {
        OSNumber::withNumber(offset, 32),
        OSNumber::withNumber(size * 8, 32)
    };
    OSObject* result;

    if (ec->evaluateObject(readECBytes, &result, params, 2) != kIOReturnSuccess || !(*data = OSDynamicCast(OSData, result))) {
        AlwaysLog("read %d bytes @ 0x%02x failed\n", size, offset);
        OSSafeReleaseNULL(result);
        return kIOReturnInvalid;
    }

    if ((*data)->getLength() != size) {
        AlwaysLog("read %d bytes @ 0x%02x, got %d bytes\n", size, offset, (*data)->getLength());
        OSSafeReleaseNULL(result);
        return kIOReturnNoBandwidth;
    }

    return kIOReturnSuccess;
}

IOReturn YogaVPC::method_we1b(UInt32 offset, UInt32 value) {
    if (!ECReadCap)
        return kIOReturnUnsupported;

    OSObject* params[2] = {
        OSNumber::withNumber(offset, 32),
        OSNumber::withNumber(value, 32)
    };
    UInt32 result;

    IOReturn ret = ec->evaluateInteger(writeECOneByte, &result, params, 2);
    if (ret != kIOReturnSuccess)
        AlwaysLog("write 0x%02x @ 0x%02x failed\n", value, offset);

    return ret;
}

bool YogaVPC::dumpECOffset(UInt32 value) {
    bool ret = false;
    UInt32 size = value >> 8;
    if (size) {
        UInt32 offset = value & 0xff;
        if (offset + size > 0x100) {
            AlwaysLog("read %d bytes @ 0x%02x exceeded\n", size, offset);
        } else {
            OSData* result;
            if (method_recb(offset, size, &result) == kIOReturnSuccess) {
                const UInt8* data = reinterpret_cast<const UInt8 *>(result->getBytesNoCopy());
                int len = 0x10;
                char *buf = new char[len*3];
                if (size > 8) {
                    AlwaysLog("%d bytes @ 0x%02x\n", size, offset);
                    for (int i = 0; i < size; i += len) {
                        memset(buf, 0, len*3);
                        for (int j = 0; j < (len < size-i ? len : size-i); j++)
                            snprintf(buf+3*j, 4, "%02x ", data[i+j]);
                        buf[len*3-1] = '\0';
                        AlwaysLog("0x%02x: %s\n", offset+i, buf);
                    }
                } else {
                    for (int j = 0; j < size; j++)
                        snprintf(buf+3*j, 4, "%02x ", data[j]);
                    buf[size*3-1] = '\0';
                    AlwaysLog("%d bytes @ 0x%02x: %s\n", size, offset, buf);
                }
                delete [] buf;
                result->release();
                ret = true;
            }
        }
    } else {
        UInt32 integer;
        if (method_re1b(value, &integer) == kIOReturnSuccess) {
            AlwaysLog("0x%02x: %02x\n", value, integer);
            ret = true;
        }
    }
    return ret;
}

