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
    bool res = super::init(dictionary);
    IOLog("%s Initializing\n", getName());

    extern kmod_info_t kmod_info;
    setProperty("YogaSMC,Build", __DATE__);
    setProperty("YogaSMC,Version", kmod_info.version);

    return res;
}

IOService *YogaVPC::probe(IOService *provider, SInt32 *score)
{
    IOLog("%s::%s Probing\n", getName(), provider->getName());

    vpc = OSDynamicCast(IOACPIPlatformDevice, provider);
    if (!vpc)
        return nullptr;

    IORegistryEntry* parent = vpc->getParentEntry(gIOACPIPlane);
    auto pnp = OSString::withCString(PnpDeviceIdEC);
    if (parent->compareName(pnp))
        ec = OSDynamicCast(IOACPIPlatformDevice, parent);
    pnp->release();

    if (!ec)
        return nullptr;

    return super::probe(provider, score);
}

bool YogaVPC::start(IOService *provider) {
    bool res = super::start(provider);

    name = provider->getName();

    IOLog("%s::%s Starting\n", getName(), name);

    if (!initVPC())
        return false;

    workLoop = IOWorkLoop::workLoop();
    commandGate = IOCommandGate::commandGate(this);
    if (!workLoop || !commandGate || (workLoop->addEventSource(commandGate) != kIOReturnSuccess)) {
        IOLog("%s::%s Failed to add commandGate\n", getName(), name);
        return false;
    }

    updateAll();

    PMinit();
    provider->joinPMtree(this);
    registerPowerDriver(this, IOPMPowerStates, kIOPMNumberPowerStates);

    setProperty(kDeliverNotifications, kOSBooleanTrue);
    registerService();
    return res;
}

bool YogaVPC::initVPC() {
    if (vpc->validateObject(getClamshellMode) == kIOReturnSuccess &&
        vpc->validateObject(setClamshellMode) == kIOReturnSuccess)
        clamshellCap = true;

    if (vpc->validateObject(setThermalControl) == kIOReturnSuccess) {
        UInt64 result;
        if (DYTCCommand(DYTC_CMD_QUERY, &result)) {
            DYTCCap = (result >> DYTC_QUERY_ENABLE_BIT) & 0x1;
            if (DYTCCap) {
                DYTCRevision = (result >> DYTC_QUERY_REV_BIT) & 0xF;
                DYTCSubRevision = (result >> DYTC_QUERY_SUBREV_BIT) & 0xF;
                OSDictionary *status = OSDictionary::withCapacity(4);
                OSObject *value;
                value = OSNumber::withNumber(DYTCRevision, 4);
                status->setObject("Revision", value);
                value->release();
                value = OSNumber::withNumber(DYTCSubRevision, 4);
                status->setObject("SubRevision", value);
                value->release();
                setProperty("DYTC", status);
                status->release();
            } else {
                setProperty("DYTC", false);
            }
            IOLog(updateSuccess, getName(), name, DYTCPrompt, DYTCCap);
        } else {
            setProperty("DYTC", "error");
            IOLog(updateFailure, getName(), name, DYTCPrompt);
        }
    }

    return true;
}

bool YogaVPC::exitVPC() {
    if (clamshellMode) {
        IOLog("%s::%s Disabling clamshell mode\n", getName(), name);
        toggleClamshell();
    }
    return true;
}

void YogaVPC::stop(IOService *provider) {
    IOLog("%s::%s Stopping\n", getName(), name);
    exitVPC();

    workLoop->removeEventSource(commandGate);
    OSSafeReleaseNULL(commandGate);
    OSSafeReleaseNULL(workLoop);

    PMstop();

    terminate();
    detach(provider);
    super::stop(provider);
}

YogaVPC* YogaVPC::withDevice(IOACPIPlatformDevice *device, OSString *pnp) {
    YogaVPC* vpc = OSTypeAlloc(YogaVPC);

    OSDictionary* dictionary = OSDictionary::withCapacity(1);
    dictionary->setObject("Type", pnp);
    
    vpc->vpc = device;

    if (!vpc->init(dictionary) ||
        !vpc->attach(device) ||
        !vpc->start(device)) {
        OSSafeReleaseNULL(vpc);
    }

    dictionary->release();

    return vpc;
}

void YogaVPC::updateAll() {
    updateBacklight();
    updateClamshell();
    updateDYTC();
}

void YogaVPC::setPropertiesGated(OSObject* props) {
    if (!vpc) {
        IOLog(VPCUnavailable, getName(), name);
        return;
    }

    OSDictionary* dict = OSDynamicCast(OSDictionary, props);
    if (!dict)
        return;

//    IOLog("%s: %d objects in properties\n", getName(), name, dict->getCount());
    OSCollectionIterator* i = OSCollectionIterator::withCollection(dict);

    if (i) {
        while (OSString* key = OSDynamicCast(OSString, i->getNextObject())) {
            if (key->isEqualTo(clamshellPrompt)) {
                OSBoolean * value = OSDynamicCast(OSBoolean, dict->getObject(clamshellPrompt));
                if (value == nullptr) {
                    IOLog(valueInvalid, getName(), name, clamshellPrompt);
                    continue;
                }

                updateClamshell(false);

                if (value->getValue() == clamshellMode) {
                    IOLog(valueMatched, getName(), name, clamshellPrompt, clamshellMode);
                } else {
                    toggleClamshell();
                }
            } else if (key->isEqualTo(backlightPrompt)) {
                OSNumber * value = OSDynamicCast(OSNumber, dict->getObject(backlightPrompt));
                if (value == nullptr) {
                    IOLog(valueInvalid, getName(), name, backlightPrompt);
                    continue;
                }

                updateBacklight(false);

                if (value->unsigned32BitValue() == backlightLevel)
                    IOLog(valueMatched, getName(), name, backlightPrompt, backlightLevel);
                else
                    setBacklight(value->unsigned32BitValue());
            } else if (key->isEqualTo(autoBacklightPrompt)) {
                OSNumber * value = OSDynamicCast(OSNumber, dict->getObject(autoBacklightPrompt));
                if (value == nullptr || value->unsigned8BitValue() > 3) {
                    IOLog(valueInvalid, getName(), name, autoBacklightPrompt);
                    continue;
                }

                if (value->unsigned8BitValue() == automaticBacklightMode) {
                    IOLog(valueMatched, getName(), name, autoBacklightPrompt, automaticBacklightMode);
                } else {
                    automaticBacklightMode = value->unsigned8BitValue();
                    setProperty(autoBacklightPrompt, automaticBacklightMode, 8);
                }
            } else if (key->isEqualTo("ReadECOffset")) {
                OSNumber * value = OSDynamicCast(OSNumber, dict->getObject("ReadECOffset"));
                if (value == nullptr) {
                    IOLog("%s::%s invalid number\n", getName(), name);
                    continue;
                }
                dumpECOffset(value->unsigned32BitValue());
            } else if (key->isEqualTo("ReadECName")) {
                OSString *value = OSDynamicCast(OSString, dict->getObject("ReadECName"));
                if (!value) {
                    IOLog("%s::%s invalid name\n", getName(), name);
                    continue;
                }
                if (value->getLength() !=4) {
                    IOLog("%s::%s invalid length %d\n", getName(), name, value->getLength());
                    continue;
                }
                UInt32 result;
                if (readECName(value->getCStringNoCopy(), &result) == kIOReturnSuccess)
                    IOLog("%s::%s %s: 0x%02x\n", getName(), name, value->getCStringNoCopy(), result);
            } else if (key->isEqualTo(DYTCPrompt)) {
                if (!DYTCCap) {
                    IOLog("%s::%s %s not supported", getName(), name, DYTCPrompt);
                    continue;
                }
                OSString * value = OSDynamicCast(OSString, dict->getObject(DYTCPrompt));
                if (value == nullptr) {
                    IOLog(valueInvalid, getName(), name, DYTCPrompt);
                    continue;
                }
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
                        IOLog(valueInvalid, getName(), name, DYTCPrompt);
                        continue;;
                }
                if (!setDYTC(mode))
                    IOLog(toggleFailure, getName(), name, DYTCPrompt);
                else
                    IOLog(toggleSuccess, getName(), name, DYTCPrompt, mode, "see ioreg");
            } else if (key->isEqualTo(updatePrompt)) {
                updateAll();
            } else {
                IOLog("%s::%s Unknown property %s\n", getName(), name, key->getCStringNoCopy());
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
        IOLog("%s::%s message: type=%x, provider=%s, argument=0x%04x\n", getName(), name, type, provider->getName(), *((UInt32 *) argument));
        updateAll();
    } else {
        IOLog("%s::%s message: type=%x, provider=%s\n", getName(), name, type, provider->getName());
    }
    return kIOReturnSuccess;
}

bool YogaVPC::updateClamshell(bool update) {
    if (!clamshellCap)
        return true;

    UInt32 state;

    if (vpc->evaluateInteger(getClamshellMode, &state) != kIOReturnSuccess) {
        IOLog(updateFailure, getName(), name, clamshellPrompt);
        return false;
    }

    clamshellMode = state ? true : false;

    if (update) {
        IOLog(updateSuccess, getName(), name, clamshellPrompt, state);
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
        IOLog(toggleFailure, getName(), name, clamshellPrompt);
        return false;
    }

    clamshellMode = !clamshellMode;
    IOLog(toggleSuccess, getName(), name, clamshellPrompt, clamshellMode, (clamshellMode ? "on" : "off"));
    setProperty(clamshellPrompt, clamshellMode);

    return true;
}

IOReturn YogaVPC::setPowerState(unsigned long powerState, IOService *whatDevice){
    IOLog("%s::%s powerState %ld : %s\n", getName(), name, powerState, powerState ? "on" : "off");

    if (whatDevice != this)
        return kIOReturnInvalid;

    if (backlightCap && automaticBacklightMode & 0x1) {
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
        IOLog(toggleFailure, getName(), name, DYTCPrompt);
        return false;
    }
#ifdef DEBUG
    IOLog("%s::%s %s 0x%llx 0x%llx\n", getName(), name, DYTCPrompt, cmd, *result);
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
    value = OSNumber::withNumber(DYTCRevision, 4);
    status->setObject("Revision", value);
    value->release();
    value = OSNumber::withNumber(DYTCSubRevision, 4);
    status->setObject("SubRevision", value);
    value->release();

    int funcmode = (result >> DYTC_GET_FUNCTION_BIT) & 0xF;
    switch (funcmode) {
        case DYTC_FUNCTION_STD:
            value = OSString::withCString("Standard");
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
            value = OSString::withCString("Lap");
            break;

        case DYTC_FUNCTION_MMC:
            value = OSString::withCString("Desk");
            break;

        default:
            IOLog(valueUnknown, getName(), name, DYTCFuncPrompt, funcmode);
            char Unknown[10];
            snprintf(Unknown, 10, "Unknown:%1x", funcmode);
            value = OSString::withCString(Unknown);
            break;
    }
    status->setObject("FuncMode", value);
    value->release();

    int perfmode = (result >> DYTC_GET_MODE_BIT) & 0xF;
    switch (perfmode) {
        case DYTC_MODE_PERFORM:
            if (funcmode == DYTC_FUNCTION_CQL)
                value = OSString::withCString("Performance (Reduced as lapmode active)");
            else
                value = OSString::withCString("Performance");
            break;

        case DYTC_MODE_QUIET:
            value = OSString::withCString("Quiet");
            break;

        case DYTC_MODE_BALANCE:
            value = OSString::withCString("Balance");
            break;

        default:
            IOLog(valueUnknown, getName(), name, DYTCPerfPrompt, perfmode);
            char Unknown[10];
            snprintf(Unknown, 10, "Unknown:%1x", perfmode);
            value = OSString::withCString(Unknown);
            break;
    }
    status->setObject("PerfMode", value);
    value->release();
    setProperty("DYTC", status);
    status->release();

    if (update)
        IOLog("%s::%s %s 0x%llx\n", getName(), name, DYTCPrompt, result);
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
        IOLog("%s findPNP failed\n", getName());
        return nullptr;
    }
    auto pnp = OSString::withCString(id);

    while (auto entry = iterator->getNextObject()) {
        if (entry->compareName(pnp)) {
            *dev = OSDynamicCast(IOACPIPlatformDevice, entry);
            if (*dev) {
                IOLog("%s %s available at %s\n", getName(), id, (*dev)->getName());
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
            IOLog("%s::%s read %s failed, bad argument (field size too large?)\n", getName(), name, name);
            break;
            
        default:
            IOLog("%s::%s read %s failed %x\n", getName(), name, name, ret);
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
        IOLog("%s::%s read 0x%02x failed\n", getName(), name, offset);

    return ret;
}

IOReturn YogaVPC::method_recb(UInt32 offset, UInt32 size, UInt8 *data) {
    // Arg0 - offset in bytes from zero-based EC
    // Arg1 - size of buffer in bits
    OSObject* params[2] = {
        OSNumber::withNumber(offset, 32),
        OSNumber::withNumber(size * 8, 32)
    };
    OSObject* result;

    IOReturn ret = ec->evaluateObject(readECBytes, &result, params, 2);
    if (ret != kIOReturnSuccess) {
        IOLog("%s::%s read %d bytes @ 0x%02x failed\n", getName(), name, size, offset);
        return ret;
    }

    OSData* osdata = OSDynamicCast(OSData, result);
    if (!data) {
        IOLog("%s::%s read %d bytes @ 0x%02x invalid\n", getName(), name, size, offset);
        return kIOReturnNotReadable;
    }

    if (osdata->getLength() != size) {
        IOLog("%s::%s read %d bytes @ 0x%02x, got %d bytes\n", getName(), name, size, offset, osdata->getLength());
        return kIOReturnNoBandwidth;
    }

    memcpy(data, osdata->getBytesNoCopy(), size);

    result->release();
    return ret;
}

IOReturn YogaVPC::method_we1b(UInt32 offset, UInt32 value) {
    OSObject* params[2] = {
        OSNumber::withNumber(offset, 32),
        OSNumber::withNumber(value, 32)
    };
    UInt32 result;

    IOReturn ret = ec->evaluateInteger(writeECOneByte, &result, params, 2);
    if (ret != kIOReturnSuccess)
        IOLog("%s::%s write 0x%02x @ 0x%02x failed\n", getName(), name, value, offset);

    return ret;
}

void YogaVPC::dumpECOffset(UInt32 value) {
    UInt32 size = value >> 8;
    if (size) {
        UInt32 offset = value & 0xff;
        if (offset + size > 0x100) {
            IOLog("%s::%s read %d bytes @ 0x%02x exceeded\n", getName(), name, size, offset);
            return;
        }

        UInt8 *data = new UInt8[size];
        if (method_recb(offset, size, data) == kIOReturnSuccess) {
            int len = 0x10;
            char *buf = new char[len*3];
            if (size > 8) {
                IOLog("%s::%s %d bytes @ 0x%02x\n", getName(), name, size, offset);
                for (int i = 0; i < size; i += len) {
                    memset(buf, 0, len*3);
                    for (int j = 0; j < (len < size-i ? len : size-i); j++)
                        snprintf(buf+3*j, 4, "%02x ", data[i+j]);
                    buf[len*3-1] = '\0';
                    IOLog("%s::%s 0x%02x: %s\n", getName(), name, offset+i, buf);
                }
            } else {
                for (int j = 0; j < size; j++)
                    snprintf(buf+3*j, 4, "%02x ", data[j]);
                buf[size*3-1] = '\0';
                IOLog("%s::%s %d bytes @ 0x%02x: %s\n", getName(), name, size, offset, buf);
            }
            delete [] buf;
        }
        delete [] data;
    } else {
        UInt32 integer;
        if (method_re1b(value, &integer) == kIOReturnSuccess)
            IOLog("%s::%s 0x%02x: %02x\n", getName(), name, value, integer);
    }
}

