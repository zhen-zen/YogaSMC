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

#include "../YogaSMC.hpp"
#include "../YogaVPC/ThinkVPC.hpp"

class ThinkWMI : public YogaWMI
{
    typedef YogaWMI super;
    OSDeclareDefaultStructors(ThinkWMI)

private:
    bool initVPC() APPLE_KEXT_OVERRIDE;
    inline OSString *getPnp() APPLE_KEXT_OVERRIDE {return OSString::withCString(PnpDeviceIdVPCThink);};

    ThinkVPC *dev;

public:
    IOService *probe(IOService *provider, SInt32 *score) APPLE_KEXT_OVERRIDE;
    void stop(IOService *provider) APPLE_KEXT_OVERRIDE;
    void free(void) APPLE_KEXT_OVERRIDE;
};

#endif /* ThinkWMI_hpp */
