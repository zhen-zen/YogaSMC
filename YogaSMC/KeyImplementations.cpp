//
//  KeyImplementations.cpp
//  YogaSMC
//
//  Created by Zhen on 7/28/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "KeyImplementations.hpp"
#include "YogaSMC.hpp"

SMC_RESULT BDVT::writeAccess() {
    YogaSMC *drv = OSDynamicCast(YogaSMC, dst);
    if (!drv)
        return SmcNotWritable;
    return SmcSuccess;
}

SMC_RESULT BDVT::update(const SMC_DATA *src) {
    bool newValue = *(reinterpret_cast<const bool *>(src));
    bool *oldValue = reinterpret_cast<bool *>(data);
    YogaSMC *drv = OSDynamicCast(YogaSMC, dst);
    DBGLOG("vpckey", "BDVT update %d -> %d", *oldValue, newValue);
    if (drv)
        drv->dispatchMessage(kSMC_setConservation, &newValue);
    *oldValue = newValue;
    return SmcSuccess;
}

SMC_RESULT CH0B::writeAccess() {
    // TODO: determine whether charger control is supported
    return SmcSuccess;
}

SMC_RESULT CH0B::update(const SMC_DATA *src) {
    DBGLOG("vpckey", "CH0B update 0x%x -> 0x%x", data[0], src[0]);
    lilu_os_memcpy(data, src, 1);
    return SmcSuccess;
}

SMC_RESULT simpleECKey::readAccess() {
    uint16_t *ptr = reinterpret_cast<uint16_t *>(data);
    UInt32 result;
    if (ec->evaluateInteger(method, &result) != kIOReturnSuccess) {
        SYSLOG("vpckey", "%s update failed", method);
        return SmcNotReadable;
    }
    *ptr = VirtualSMCAPI::encodeSp(SmcKeyTypeSp78, result);
    DBGLOG("vpckey", "%s update 0x%x", method, result);
    return SmcSuccess;
}
