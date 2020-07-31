//  SPDX-License-Identifier: GPL-2.0-only
//
//  ThinkWMI.hpp
//  YogaSMC
//
//  Created by Zhen on 6/27/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef ThinkWMI_hpp
#define ThinkWMI_hpp

#include "YogaWMI.hpp"

class ThinkWMI : public YogaWMI
{
    typedef YogaWMI super;
    OSDeclareDefaultStructors(ThinkWMI)

private:
    inline OSString *getPnp() APPLE_KEXT_OVERRIDE {return OSString::withCString(PnpDeviceIdVPCThink);};

public:
};

#endif /* ThinkWMI_hpp */
