//  SPDX-License-Identifier: GPL-2.0-only
//
//  IdeaWMI.hpp
//  YogaSMC
//
//  Created by Zhen on 2020/6/23.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef IdeaWMI_hpp
#define IdeaWMI_hpp

#include "../YogaSMC.hpp"
#include "../YogaVPC/IdeaVPC.hpp"

class IdeaWMI : public YogaWMI
{
    typedef YogaWMI super;
    OSDeclareDefaultStructors(IdeaWMI)

private:
    bool initVPC() APPLE_KEXT_OVERRIDE;
    inline OSString *getPnp() APPLE_KEXT_OVERRIDE {return OSString::withCString(PnpDeviceIdVPCIdea);};

    void YogaEvent(UInt32 argument) APPLE_KEXT_OVERRIDE;

    IdeaVPC *dev;

public:
    IOService *probe(IOService *provider, SInt32 *score) APPLE_KEXT_OVERRIDE;
    void stop(IOService *provider) APPLE_KEXT_OVERRIDE;
    void free(void) APPLE_KEXT_OVERRIDE;
    IOReturn setPowerState(unsigned long powerState, IOService * whatDevice) APPLE_KEXT_OVERRIDE;
};

#endif /* IdeaWMI_hpp */
