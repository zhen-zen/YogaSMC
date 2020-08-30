//
//  ThinkSMC.hpp
//  YogaSMC
//
//  Created by Zhen Gong on 8/30/20.
//  Copyright © 2020 zhen. All rights reserved.
//

#ifndef ThinkSMC_hpp
#define ThinkSMC_hpp

#include "YogaSMC.hpp"

class ThinkSMC : public YogaSMC
{
    typedef YogaSMC super;
    OSDeclareDefaultStructors(ThinkSMC)

private:
    void addVSMCKey() APPLE_KEXT_OVERRIDE;

public:
    static ThinkSMC *withDevice(IOService *provider, IOACPIPlatformDevice *device);

};

#endif /* ThinkSMC_hpp */
