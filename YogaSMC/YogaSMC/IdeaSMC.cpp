//  SPDX-License-Identifier: GPL-2.0-only
//
//  IdeaSMC.cpp
//  YogaSMC
//
//  Created by Zhen on 8/25/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "IdeaSMC.hpp"
OSDefineMetaClassAndStructors(IdeaSMC, YogaSMC);

IdeaSMC* IdeaSMC::withDevice(IOService *provider, IOACPIPlatformDevice *device) {
    IdeaSMC* dev = OSTypeAlloc(IdeaSMC);

    OSDictionary *dictionary = OSDictionary::withCapacity(1);
    OSDictionary *sensors = OSDictionary::withDictionary(OSDynamicCast(OSDictionary, provider->getProperty("Sensors")));
    dictionary->setObject("Sensors", sensors);

    dev->ec = device;

    if (!dev->init(dictionary) ||
        !dev->attach(provider)) {
        OSSafeReleaseNULL(dev);
    }

    dev->conf = sensors;
    sensors->release();
    dictionary->release();
    return dev;
}

void IdeaSMC::addVSMCKey() {
    super::addVSMCKey();
}
