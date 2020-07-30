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

#include "YogaWMI.hpp"

class IdeaWMI : public YogaWMI
{
    typedef YogaWMI super;
    OSDeclareDefaultStructors(IdeaWMI)

private:
    inline OSString *getPnp() APPLE_KEXT_OVERRIDE {return OSString::withCString(PnpDeviceIdVPCIdea);};

    void ACPIEvent(UInt32 argument) APPLE_KEXT_OVERRIDE;

public:
    IOService *probe(IOService *provider, SInt32 *score) APPLE_KEXT_OVERRIDE;
};

#endif /* IdeaWMI_hpp */
