//  SPDX-License-Identifier: GPL-2.0-only
//
//  IdeaSMC.hpp
//  YogaSMC
//
//  Created by Zhen on 8/25/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef IdeaSMC_hpp
#define IdeaSMC_hpp

#include "YogaSMC.hpp"

class IdeaSMC : public YogaSMC
{
    typedef YogaSMC super;
    OSDeclareDefaultStructors(IdeaSMC)

private:
    void addVSMCKey() APPLE_KEXT_OVERRIDE;

public:
    static IdeaSMC *withDevice(IOService *provider, IOACPIPlatformDevice *device);

};

#endif /* IdeaSMC_hpp */
