//
//  DYVPC.hpp
//  YogaSMC
//
//  Created by Zhen on 1/7/21.
//  Copyright © 2021 Zhen. All rights reserved.
//

#ifndef DYVPC_hpp
#define DYVPC_hpp

#include "YogaVPC.hpp"

#define INPUT_WMI_EVENT "95F24279-4D7B-4334-9387-ACCDC67EF61C"
#define BIOS_QUERY_WMI_METHOD "5FB7F034-2C63-45e9-BE91-3D44E2C707E4"

class DYVPC : public YogaVPC
{
    typedef YogaVPC super;
    OSDeclareDefaultStructors(DYVPC)

private:
    bool probeVPC(IOService *provider) APPLE_KEXT_OVERRIDE;
    bool initVPC() APPLE_KEXT_OVERRIDE;
//    void setPropertiesGated(OSObject* props) APPLE_KEXT_OVERRIDE;
//    void updateAll() APPLE_KEXT_OVERRIDE;
    void updateVPC() APPLE_KEXT_OVERRIDE;
    bool exitVPC() APPLE_KEXT_OVERRIDE;

    /**
     *  input capability
     */
    bool inputCap {false};

    /**
     *  BIOS setting capability
     */
    bool BIOSCap {false};

public:
    IOReturn message(UInt32 type, IOService *provider, void *argument) APPLE_KEXT_OVERRIDE;
};
#endif /* DYVPC_hpp */
