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

    OSDictionary* dictionary = OSDictionary::withCapacity(1);

    dev->ec = device;

    if (!dev->init(dictionary) ||
        !dev->attach(provider)) {
        OSSafeReleaseNULL(dev);
    }

    dev->conf = OSDynamicCast(OSDictionary, provider->getProperty("Sensors"));
    dictionary->release();
    return dev;
}

void ThinkSMC::addVSMCKey() {
    super::addVSMCKey();
}
