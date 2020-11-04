//  SPDX-License-Identifier: GPL-2.0-only
//
//  ThinkSMC.cpp
//  YogaSMC
//
//  Created by Zhen on 8/30/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "ThinkSMC.hpp"

OSDefineMetaClassAndStructors(ThinkSMC, YogaSMC);

ThinkSMC* ThinkSMC::withDevice(IOService *provider, IOACPIPlatformDevice *device) {
    ThinkSMC* dev = OSTypeAlloc(ThinkSMC);

    dev->conf = OSDictionary::withDictionary(OSDynamicCast(OSDictionary, provider->getProperty("Sensors")));

    OSDictionary *dictionary = OSDictionary::withCapacity(1);
    dictionary->setObject("Sensors", dev->conf);

    dev->ec = device;
    dev->name = device->getName();

    if (!dev->init(dictionary) ||
        !dev->attach(provider)) {
        OSSafeReleaseNULL(dev);
    }

    dictionary->release();
    return dev;
}

void ThinkSMC::addVSMCKey() {
    VirtualSMCAPI::addKey(KeyFNum, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(dualFan ? 2 : 1, nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
    VirtualSMCAPI::addKey(KeyF0Ac(0), vsmcPlugin.data, VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new atomicFpKey(&fanSpeed[0])));
    if (dualFan)
        VirtualSMCAPI::addKey(KeyF0Ac(1), vsmcPlugin.data, VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new atomicFpKey(&fanSpeed[1])));
    setProperty("Dual fan", dualFan);
    super::addVSMCKey();
}

void ThinkSMC::updateEC(OSObject* owner, IOTimerEventSource* timer) {
    if (!awake)
        return;
    if (!(ECAccessCap & BIT(0))) {
        super::updateEC(owner, timer);
        return;
    }

    UInt8 lo, hi;
    if (method_re1b(0x84, &lo) == kIOReturnSuccess &&
        method_re1b(0x85, &hi) == kIOReturnSuccess) {
        UInt16 result = (hi << 8) | lo;
        atomic_store_explicit(&fanSpeed[0], result, memory_order_release);
    }

    if (!dualFan) {
        super::updateEC(owner, timer);
        return;
    }

    UInt8 select;
    if (method_re1b(0x84, &select) == kIOReturnSuccess) {
        if ((select & 0x1) != 0)
            select &= 0xfe;
        else
            select |= 0x1;
        if (method_we1b(0x31, select) == kIOReturnSuccess &&
            method_re1b(0x84, &lo) == kIOReturnSuccess &&
            method_re1b(0x85, &hi) == kIOReturnSuccess) {
            UInt16 result = (hi << 8) | lo;
            atomic_store_explicit(&fanSpeed[1], result, memory_order_release);
            if ((select & 0x1) != 0)
                select &= 0xfe;
            else
                select |= 0x1;
            if (method_we1b(0x31, select) != kIOReturnSuccess)
                dualFan = false;
        } else {
            dualFan = false;
        }
    } else {
        dualFan = false;
    }
    super::updateEC(owner, timer);
}
