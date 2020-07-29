//
//  KeyImplementations.cpp
//  YogaSMC
//
//  Created by Zhen on 7/28/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "KeyImplementations.hpp"

SMC_RESULT BDVT::readAccess() {
    bool *ptr = reinterpret_cast<bool *>(data);
    *ptr = value;
    if (vpc)
        DBGLOG("vpckey", "BDVT read %d / %d", data[0], vpc->conservationMode);
    else
        DBGLOG("vpckey", "BDVT read %d", data[0]);
    return SmcSuccess;
}

SMC_RESULT BDVT::writeAccess() {
    value = *(reinterpret_cast<bool *>(data));
    if (vpc) {
        vpc->updateConservation();
        DBGLOG("vpckey", "BDVT write %d / %d", data[0], vpc->conservationMode);
        if (value != vpc->conservationMode) {
            vpc->toggleConservation();
        }
    } else {
        DBGLOG("vpckey", "BDVT write %d", data[0]);
    }
    return SmcSuccess;
}
