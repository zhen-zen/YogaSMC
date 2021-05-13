//
//  DYVPC.cpp
//  YogaSMC
//
//  Created by Zhen on 1/7/21.
//  Copyright © 2021 Zhen. All rights reserved.
//

#include "DYVPC.hpp"
#include "YogaWMI.hpp"
#ifndef ALTER
#include "DYSMC.hpp"
#endif
OSDefineMetaClassAndStructors(DYVPC, YogaVPC);

bool DYVPC::probeVPC(IOService *provider) {
    YWMI = new WMI(provider);
    if (!YWMI->initialize())
        return false;

    inputCap = YWMI->hasMethod(INPUT_WMI_EVENT, ACPI_WMI_EVENT);
    BIOSCap = YWMI->hasMethod(BIOS_QUERY_WMI_METHOD);

    if (!inputCap && !BIOSCap) {
        delete YWMI;
        return false;
    }

    return true;
}

bool DYVPC::initEC() {
    UInt32 state, attempts = 0;

    // _REG will write Arg1 to ECRG to connect / disconnect the region
    if (ec->validateObject("ECRG") == kIOReturnSuccess) {
        do {
            if (ec->evaluateInteger("ECRG", &state) == kIOReturnSuccess && state != 0) {
                if (attempts != 0)
                    setProperty("EC Access Retries", attempts, 8);
                return true;
            }
            IOSleep(100);
        } while (++attempts < 5);
        AlwaysLog(updateFailure, "ECRG");
    }

    return true;
}

bool DYVPC::initVPC() {
    if (!initEC())
        return false;

    super::initVPC();

    YWMI->start();

    if (inputCap)
        YWMI->enableEvent(INPUT_WMI_EVENT, true);

//    if (BIOSCap) {
//        UInt32 value;
//        if (WMIQuery(HPWMI_HARDWARE_QUERY, &value))
//            setProperty("HARDWARE", value, 32);
//        if (WMIQuery(HPWMI_WIRELESS_QUERY, &value))
//            setProperty("WIRELESS", value, 32);
//        if (WMIQuery(HPWMI_WIRELESS2_QUERY, &value))
//            setProperty("WIRELESS2", value, 32);
//        if (WMIQuery(HPWMI_FEATURE2_QUERY, &value))
//            setProperty("FEATURE2", value, 32);
//        else if (WMIQuery(HPWMI_FEATURE_QUERY, &value))
//            setProperty("FEATURE", value, 32);
//        if (WMIQuery(HPWMI_POSTCODEERROR_QUERY, &value))
//            setProperty("POSTCODE", value, 32);
//        if (WMIQuery(HPWMI_THERMAL_POLICY_QUERY, &value))
//            setProperty("THERMAL_POLICY", value, 32);
//    }
    return true;
}

bool DYVPC::exitVPC() {
    if (inputCap)
        YWMI->enableEvent(INPUT_WMI_EVENT, false);

    if (YWMI)
        delete YWMI;

    return super::exitVPC();
}

void DYVPC::updateVPC(UInt32 event) {
    UInt32 id;
    UInt32 data;

    OSObject *result;
    if (!YWMI->getEventData(event, &result)) {
        AlwaysLog("message: Unknown ACPI notification 0x%04x", event);
        return;
    }

    OSData *buf = OSDynamicCast(OSData, result);
    if (!buf) {
        DebugLog("Unknown response type");
        result->release();
        return;
    }

    switch (buf->getLength()) {
        case 8:
            id = *(reinterpret_cast<UInt32 const*>(buf->getBytesNoCopy(0, 4)));
            data = *(reinterpret_cast<UInt32 const*>(buf->getBytesNoCopy(4, 4)));
            break;
            
        case 16:
            id = *(reinterpret_cast<UInt32 const*>(buf->getBytesNoCopy(0, 8)));
            data = *(reinterpret_cast<UInt32 const*>(buf->getBytesNoCopy(8, 8)));
            break;
            
        default:
            DebugLog("Unknown response length %d", buf->getLength());
            buf->release();
            return;
    }
    buf->release();

    DebugLog("Unknown id: 0x%x - 0x%x", id, data);
}

IOReturn DYVPC::message(UInt32 type, IOService *provider, void *argument) {
    if (type != kIOACPIMessageDeviceNotification || !argument)
        return super::message(type, provider, argument);

    updateVPC(*((UInt32 *) argument));

    return kIOReturnSuccess;
}

bool DYVPC::WMIQuery(UInt32 query, void *buffer, enum hp_wmi_command command, UInt32 insize, UInt32 outsize) {
    struct bios_args args = {
        .signature      = 0x55434553,
        .command        = command,
        .commandtype    = query,
        .datasize       = insize,
        .data           = { 0 },
    };

    if (insize > sizeof(args.data))
        return false;

    UInt32 mid;
    if (outsize > 4096)
        return false;
    else if (outsize > 1024)
        mid = 5;
    else if (outsize > 128)
        mid = 4;
    else if (outsize > 4)
        mid = 3;
    else if (outsize > 0)
        mid = 2;
    else
        mid = 1;

    memcpy(&args.data[0], buffer, insize);

    OSObject *params[] = {
        OSNumber::withNumber(0ULL, 32),
        OSNumber::withNumber(mid, 32),
        OSData::withBytesNoCopy(&args, sizeof(struct bios_args)),
    };

    OSObject *result;

    bool ret = YWMI->executeMethod(BIOS_QUERY_WMI_METHOD, &result, params, 3);
    params[0]->release();
    params[1]->release();
    params[2]->release();

    if (!ret) {
        AlwaysLog("BIOS query 0x%x: evaluation failed", query);
        OSSafeReleaseNULL(result);
        return false;
    }

    OSData *output = OSDynamicCast(OSData, result);
    if (output == nullptr) {
        AlwaysLog("BIOS query 0x%x: unexpected output type", query);
        OSSafeReleaseNULL(result);
        return false;
    }

    const struct bios_return *biosRet = reinterpret_cast<const struct bios_return*>(output->getBytesNoCopy());
    switch (biosRet->return_code) {
        case 0:
            break;
            
        case HPWMI_RET_UNKNOWN_COMMAND:
            DebugLog("BIOS query 0x%x: unknown COMMAND", query);
            ret = false;
            break;
            
        case HPWMI_RET_UNKNOWN_CMDTYPE:
            DebugLog("BIOS query 0x%x: unknown CMDTYPE", query);
            ret = false;
            break;
            
        default:
            AlwaysLog("BIOS query 0x%x: return code error - %d", query, biosRet->return_code);
            ret = false;
            break;
    }

    if (ret && outsize != 0) {
        memset(buffer, 0, outsize);
        outsize = min(outsize, output->getLength() - sizeof(*biosRet));
        memcpy(buffer, output->getBytesNoCopy(sizeof(*biosRet), outsize), outsize);
    }

    output->release();
    return ret;
}

void DYVPC::setPropertiesGated(OSObject *props) {
    OSDictionary *dict = OSDynamicCast(OSDictionary, props);
    if (!dict)
        return;

//    AlwaysLog("%d objects in properties", dict->getCount());
    OSCollectionIterator* i = OSCollectionIterator::withCollection(dict);

    if (i) {
        while (OSString* key = OSDynamicCast(OSString, i->getNextObject())) {
            if (key->isEqualTo("BIOSQuery")) {
                if (!BIOSCap) {
                    AlwaysLog(notSupported, "BIOSQuery");
                    continue;
                }

                OSNumber *value;
                getPropertyNumber("BIOSQuery");

                UInt32 result;

                if (WMIQuery(value->unsigned32BitValue(), &result))
                    AlwaysLog("%s 0x%x result: 0x%x", "BIOSQuery", value->unsigned32BitValue(), result);
                else
                    AlwaysLog("%s failed 0x%x", "BIOSQuery", value->unsigned32BitValue());
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

bool DYVPC::examineWMI(IOService *provider) {
#ifndef ALTER
    OSString *feature;
    if ((feature = OSDynamicCast(OSString, provider->getProperty("Feature"))) &&
        feature->isEqualTo("Sensor")) {
        DYSMC *sensor = OSDynamicCast(DYSMC, smc);
        if (sensor)
            sensor->setWMI(provider);
    }
#endif
    return true;
}

IOService* DYVPC::initWMI(WMI *instance) {
    return YogaWMI::withDYWMI(instance);
}

#ifndef ALTER
IOService* DYVPC::initSMC() {
    return DYSMC::withDevice(this, ec);
}
#endif
