//
//  MateVPC.cpp
//  YogaSMC
//
//  Created by Zhen on 5/8/21.
//  Copyright © 2021 Zhen. All rights reserved.
//

#include "MateVPC.hpp"
OSDefineMetaClassAndStructors(MateVPC, YogaVPC);

bool MateVPC::probeVPC(IOService *provider) {
    YWMI = new WMI(provider);
    if (!YWMI->initialize() ||
        !YWMI->hasMethod(HWMI_BIOS_METHOD, ACPI_WMI_METHOD) ||
        !YWMI->hasMethod(HWMI_HIT_EVENT, ACPI_WMI_EVENT)) {
        delete YWMI;
        return false;
    }
    return true;
}

bool MateVPC::initVPC() {
    if (!initEC())
        return false;

    super::initVPC();

    bool ret;
    OSObject *result;

    union hwmi_arg arg;
    arg.args[0] = 1;
    arg.args[1] = 1;

    OSObject* params[3] = {
        OSNumber::withNumber(0ULL, 32),
        OSNumber::withNumber(0ULL, 32),
        OSData::withBytesNoCopy(&arg, sizeof(arg))
    };

    ret = YWMI->executeMethod(HWMI_BIOS_METHOD, &result, params, 3);
    params[0]->release();
    params[1]->release();
    params[2]->release();

    if (ret && (result))
        setProperty("GetVersion", result);

    return true;
}

bool MateVPC::initEC() {
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

    return true;
}
