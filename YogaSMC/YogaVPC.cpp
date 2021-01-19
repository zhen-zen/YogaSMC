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
 
    DebugLog("Probing");

    if ((vpc = OSDynamicCast(IOACPIPlatformDevice, provider))) {
        IORegistryEntry* parent = vpc->getParentEntry(gIOACPIPlane);
        auto pnp = OSString::withCString(PnpDeviceIdEC);
        if (parent->compareName(pnp))
            ec = OSDynamicCast(IOACPIPlatformDevice, parent);
        pnp->release();
    }

    if (!ec && !findPNP(PnpDeviceIdEC, &ec))
        return nullptr;

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
                if (strncmp(dev->getName(), "WTBT", sizeof("WTBT")) == 0) {
                    DebugLog("Skip Thunderbolt interface");
                    continue;
                }
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
                if (value->getValue() == clamshellMode) {
                    DebugLog(valueMatched, clamshellPrompt, clamshellMode);
                } else {
                    toggleClamshell();
                }
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
                if (raw != nullptr && (raw->unsigned32BitValue() < 1 || raw->unsigned32BitValue() > 8)) {
                    DYTC_CMD cmd = {.raw = raw->unsigned32BitValue()};
                    DYTC_RESULT res;
                    if (DYTCCommand(cmd, &res) && parseDYTC(res))
                        AlwaysLog(toggleSuccess, DYTCPrompt, raw->unsigned32BitValue(), "see ioreg");
                    else
                        AlwaysLog(toggleFailure, DYTCPrompt);
                    continue;
                }
                
                int mode;
                if (raw != nullptr)
                    mode = raw->unsigned32BitValue(); // 1-8
                else {
                    OSString *value;
                    getPropertyString(DYTCPrompt);
                    switch (value->getChar(0)) {
                        case 'l':
                        case 'L':
                        case 'q':
                        case 'Q':
                            mode = DYTC_MODE_NEW_1;
                            break;

                        case 'b':
                        case 'B':
                        case 'm':
                        case 'M':
                            
                        case 'd':
                        case 'D':
                            mode = DYTC_MODE_NEW_DEFAULT;
                            break;

                        case 'h':
                        case 'H':
                        case 'p':
                        case 'P':
                            mode = DYTC_MODE_NEW_8;
                            break;
                            
    
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                            mode = DYTC_MODE_NEW_1 + (value->getChar(0) - '1');
                            break;

                        default:
                            AlwaysLog(valueInvalid, DYTCPrompt);
                            continue;
                    }
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
    DebugLog("powerState %ld : %s", powerState, powerState ? "on" : "off");

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

    if (vpc->evaluateInteger(setThermalControl, &(result->raw), params, 1) != kIOReturnSuccess) {
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
            setPropertyString(status, "FuncMode", "Lap (Reduced thermals)");
            break;
        case DYTC_FUNCTION_MMC:
            setPropertyString(status, "FuncMode", "Controlled");
            break;
        case DYTC_FUNCTION_PSC:
            setPropertyString(status, "FuncMode", "Controlled");
            break;
        default:
            AlwaysLog(valueUnknown, DYTCFuncPrompt, result.get.funcmode);
            char Unknown[10];
            snprintf(Unknown, 10, "Unknown:%1x", result.get.funcmode);
            setPropertyString(status, "FuncMode", Unknown);
            break;
    }
    
    // map old codes to new ones
    if (result.get.funcmode == DYTC_FUNCTION_MMC && result.get.perfmode == DYTC_MODE_QUIET)
        result.get.perfmode = DYTC_MODE_NEW_1;
    else if (result.get.funcmode == DYTC_FUNCTION_MMC && result.get.perfmode == DYTC_MODE_PERFORM)
        result.get.perfmode = DYTC_MODE_NEW_8;
    else if (result.get.funcmode == DYTC_FUNCTION_MMC && result.get.perfmode == DYTC_MODE_BALANCE)
        result.get.perfmode = DYTC_MODE_NEW_DEFAULT;
    
    switch (result.get.perfmode) {
        case DYTC_MODE_NEW_1:
        case DYTC_MODE_NEW_2:
        case DYTC_MODE_NEW_3:
        case DYTC_MODE_NEW_4: {
            char pm[20];
            snprintf(pm, sizeof(pm), "Quiet %d", result.get.perfmode - DYTC_MODE_NEW_1 + 1);
            setPropertyString(status, "PerfMode", pm);
            break;
        }
            
        case DYTC_MODE_NEW_DEFAULT: {
            setPropertyString(status, "PerfMode", "Standard");
            break;
        }
            
        case DYTC_MODE_NEW_5:
        case DYTC_MODE_NEW_6:
        case DYTC_MODE_NEW_7:
        case DYTC_MODE_NEW_8: {
            char pm[20];
            snprintf(pm, sizeof(pm), "Performance %d", result.get.perfmode - DYTC_MODE_NEW_5 + 1);
            setPropertyString(status, "PerfMode", pm);
            break;
        }
            

            
        default:
            AlwaysLog(valueUnknown, DYTCPerfPrompt, result.get.perfmode);
            char Unknown[10];
            snprintf(Unknown, sizeof(Unknown), "Unknown:%1x", result.get.perfmode);
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
                    
                case DYTC_FUNCTION_PSC:
                    DebugLog("Found DYTC_FUNCTION_PSC");
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

bool YogaVPC::setDYTC(int newPerfMode) {
    if (!DYTCCap)
        return true;
    
    DYTC_RESULT result;
    // retrieve current state
    if (!DYTCCommand(dytc_get_cmd, &result))
        return false;
    
    int curFuncMode = result.get.funcmode;
    int curPerfMode = result.get.perfmode;
    
    bool lapMode = false;
    if (curFuncMode == DYTC_FUNCTION_CQL) {
        // the result of disable command will contain real funcmode & perfmode
        // the result of enable command will contain lapmode funcmode
        DYTC_RESULT temp;
        if (!DYTCCommand(dytc_set_cmd, &temp, DYTC_FUNCTION_CQL, 0xf, false))
            return false;
        curFuncMode = temp.get.funcmode;
        curPerfMode = temp.get.perfmode;
        lapMode = true;
    }
        
    // map MMC(0xB) codes to PSC(0xD) codes
    if (curFuncMode != DYTC_FUNCTION_PSC && curPerfMode == DYTC_MODE_QUIET)
        curPerfMode = DYTC_MODE_NEW_1;
    else if (curFuncMode != DYTC_FUNCTION_PSC && curPerfMode == DYTC_MODE_PERFORM)
        curPerfMode = DYTC_MODE_NEW_8;
    else if (curFuncMode != DYTC_FUNCTION_PSC && curPerfMode == DYTC_MODE_BALANCE)
        curPerfMode = DYTC_MODE_NEW_DEFAULT;
    
    
    if (curFuncMode == DYTC_FUNCTION_PSC) {
        // psc mode tends to be glitchy (at least on my laptop), and can lock up cpu in low mhz state so we need to bombard it with resets first
        // and yes, 1 reset is not enough at some cases so why not hold up kernel while we bombard it with resets :)))
        DYTC_RESULT temp;
        for (int a = 0; a < 20; a++) {
            if (!DYTCCommand(dytc_set_cmd, &temp, DYTC_FUNCTION_PSC, 0xf, false))
                return false;
            if (!DYTCCommand(dytc_reset_cmd, &temp))
                return false;
        }
        
    }
    
    {
        DYTC_RESULT temp;
        // try reset if we wanna go to default
        if (newPerfMode == DYTC_MODE_NEW_DEFAULT && !DYTCCommand(dytc_reset_cmd, &temp))
            return false;
        // try both new & old if we wanna control
        else if (newPerfMode != DYTC_MODE_NEW_DEFAULT && !DYTCCommand(dytc_set_cmd, &temp, DYTC_FUNCTION_PSC, newPerfMode, true) &&
            !DYTCCommand(dytc_set_cmd, &temp, DYTC_FUNCTION_MMC, newPerfMode < DYTC_MODE_NEW_5 ? DYTC_MODE_QUIET : DYTC_MODE_PERFORM, true)) {
            return false;
        }
    }
    
    
    // if we were in lapmode, enable that back and fetch the result that should have funcmode set back to lapmode (so that parse can show correct func)
    if (lapMode && !DYTCCommand(dytc_set_cmd, &result, DYTC_FUNCTION_CQL, 0xf, true))
        return false;
    // if we weren't in lapmode, retrieve the altered mode just to make sure we really got it
    else if (!lapMode && !DYTCCommand(dytc_get_cmd, &result))
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

