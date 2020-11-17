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

void IdeaSMC::addVSMCKey() {
    // Add message-based key
    VirtualSMCAPI::addKey(KeyBDVT, vsmcPlugin.data, VirtualSMCAPI::valueWithFlag(true, new BDVT(this), SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE));
    VirtualSMCAPI::addKey(KeyCH0B, vsmcPlugin.data, VirtualSMCAPI::valueWithData(nullptr, 1, SmcKeyTypeHex, new CH0B, SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE));

    super::addVSMCKey();
}
