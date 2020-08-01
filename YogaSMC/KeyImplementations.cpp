//
//  KeyImplementations.cpp
//  YogaSMC
//
//  Created by Zhen on 7/28/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "KeyImplementations.hpp"
#include "YogaSMC.hpp"

SMC_RESULT BDVT::readAccess() {
    readcount++;
    bool *ptr = reinterpret_cast<bool *>(data);
    *ptr = value;
    YogaSMC *drv = OSDynamicCast(YogaSMC, vpc);
    if (drv) {
        drv->dispatchMessage(kSMC_getConservation, ptr);
        DBGLOG("vpckey", "%d BDVT read %d / %d", readcount, value, data[0]);
        value = *ptr;
    } else {
        DBGLOG("vpckey", "%d BDVT read %d (no drv)", readcount, data[0]);
    }
    return SmcSuccess;
}

SMC_RESULT BDVT::writeAccess() {
    writecount++;
    DBGLOG("vpckey", "%d BDVT write %d -> %d", writecount, value, data[0]);
    bool *ptr = reinterpret_cast<bool *>(data);
    if (value != *ptr) {
        YogaSMC *drv = OSDynamicCast(YogaSMC, vpc);
        if (drv)
            drv->dispatchMessage(kSMC_setConservation, ptr);
        value = *ptr;
    }
    return SmcSuccess;
}

SMC_RESULT CH0B::readAccess() {
    data[0] = value;
    DBGLOG("vpckey", "CH0B read %d", value);
    return SmcSuccess;
}

SMC_RESULT CH0B::writeAccess() {
    value = data[0];
    DBGLOG("vpckey", "CH0B write %d", data[0]);
    return SmcSuccess;
}
