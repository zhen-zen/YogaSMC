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
    if (!device)
        return nullptr;

    IdeaSMC* drv = OSTypeAlloc(IdeaSMC);

    drv->conf = OSDictionary::withDictionary(OSDynamicCast(OSDictionary, provider->getProperty("Sensors")));

    OSDictionary *dictionary = OSDictionary::withCapacity(1);
    dictionary->setObject("Sensors", drv->conf);

    drv->ec = device;
    drv->iname = device->getName();

    if (!drv->init(dictionary))
        OSSafeReleaseNULL(drv);

    dictionary->release();
    return drv;
}

void IdeaSMC::addVSMCKey() {
    // Add message-based key
    VirtualSMCAPI::addKey(KeyBDVT, vsmcPlugin.data, VirtualSMCAPI::valueWithFlag(true, new BDVT(this), SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE));
    VirtualSMCAPI::addKey(KeyCH0B, vsmcPlugin.data, VirtualSMCAPI::valueWithData(nullptr, 1, SmcKeyTypeHex, new CH0B, SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE));

    if (!wmig) {
        super::addVSMCKey();
        return;
    }

    UInt32 value;
    if (wmig->getGamzeZoneData(GAME_ZONE_WMI_GET_FAN_NUM, &value) && value) {
        VirtualSMCAPI::addKey(KeyF0Ac(0), vsmcPlugin.data, VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new atomicFpKey(&currentSensor[sensorCount])));
        sensorIndex[sensorCount++] = GAME_ZONE_WMI_GET_FAN_ONE;

        if (value > 1) {
            VirtualSMCAPI::addKey(KeyF0Ac(1), vsmcPlugin.data, VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new atomicFpKey(&currentSensor[sensorCount])));
            sensorIndex[sensorCount++] = GAME_ZONE_WMI_GET_FAN_TWO;
        }

        VirtualSMCAPI::addKey(KeyFNum, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(sensorCount, nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));

        if (wmig->getGamzeZoneData(GAME_ZONE_WMI_GET_FAN_MAX, &value)) {
            VirtualSMCAPI::addKey(KeyF0Mx(0), vsmcPlugin.data, VirtualSMCAPI::valueWithFp(value, SmcKeyTypeFpe2, nullptr, SMC_KEY_ATTRIBUTE_WRITE | SMC_KEY_ATTRIBUTE_READ));
            if (sensorCount == 2)
                VirtualSMCAPI::addKey(KeyF0Mx(1), vsmcPlugin.data, VirtualSMCAPI::valueWithFp(value, SmcKeyTypeFpe2, nullptr, SMC_KEY_ATTRIBUTE_WRITE | SMC_KEY_ATTRIBUTE_READ));
        }
    }

    if (wmig->getGamzeZoneData(GAME_ZONE_WMI_GET_TMP_IR, &value) && value) {
        VirtualSMCAPI::addKey(KeyTPCD, vsmcPlugin.data, VirtualSMCAPI::valueWithSp(0, SmcKeyTypeSp78, new atomicSpKey(&currentSensor[sensorCount])));
        sensorIndex[sensorCount++] = GAME_ZONE_WMI_GET_TMP_IR;
    }

    if (wmig->getGamzeZoneData(GAME_ZONE_WMI_GET_TMP_CPU, &value) && value) {
        VirtualSMCAPI::addKey(KeyTCXC, vsmcPlugin.data, VirtualSMCAPI::valueWithSp(0, SmcKeyTypeSp78, new atomicSpKey(&currentSensor[sensorCount])));
        sensorIndex[sensorCount++] = GAME_ZONE_WMI_GET_TMP_CPU;
    }

    if (wmig->getGamzeZoneData(GAME_ZONE_WMI_GET_TMP_GPU, &value) && value) {
        VirtualSMCAPI::addKey(KeyTCGC, vsmcPlugin.data, VirtualSMCAPI::valueWithSp(0, SmcKeyTypeSp78, new atomicSpKey(&currentSensor[sensorCount])));
        sensorIndex[sensorCount++] = GAME_ZONE_WMI_GET_TMP_GPU;
    }

    super::addVSMCKey();
}

void IdeaSMC::updateEC() {
    if (!awake)
        return;

    UInt32 value;
    for (UInt8 index = 0; index < ECSensorBase; ++index)
        if (wmig->getGamzeZoneData(sensorIndex[index], &value))
            atomic_store_explicit(&currentSensor[index], value, memory_order_release);

    super::updateEC();
}

void IdeaSMC::setWMI(IOService *instance) {
    if (wmig)
        return;

    wmig = OSDynamicCast(IdeaWMIGameZone, instance);
}
