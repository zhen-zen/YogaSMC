//
//  KeyImplementations.cpp
//  YogaSMC
//
//  Created by Zhen on 7/28/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "KeyImplementations.hpp"
#include "IdeaVPC.hpp"

SMC_RESULT BDVT::readAccess() {
    bool *ptr = reinterpret_cast<bool *>(data);
    *ptr = value;
    DBGLOG("vpckey", "BDVT read %d", data[0]);//, *(IdeaVPC::getConservation()));
    return SmcSuccess;
}

SMC_RESULT BDVT::writeAccess() {
    value = *(reinterpret_cast<bool *>(data));
    DBGLOG("vpckey", "BDVT write %d", data[0]);
    return SmcSuccess;
}
