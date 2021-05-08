//
//  MateVPC.hpp
//  YogaSMC
//
//  Created by Zhen on 5/8/21.
//  Copyright © 2021 Zhen. All rights reserved.
//

#ifndef MateVPC_hpp
#define MateVPC_hpp

#include "YogaVPC.hpp"

#define HWMI_BIOS_METHOD    "ABBC0F5B-8EA1-11D1-A000-C90629100000"
#define HWMI_HIT_EVENT      "ABBC0F5C-8EA1-11D1-A000-C90629100000"

union hwmi_arg {
    UInt64 cmd;
    UInt8 args[8];
};

class MateVPC : public YogaVPC
{
    typedef YogaVPC super;
    OSDeclareDefaultStructors(MateVPC)

private:
    bool probeVPC(IOService *provider) APPLE_KEXT_OVERRIDE;
    bool initVPC() APPLE_KEXT_OVERRIDE;
//    void updateVPC(UInt32 event) APPLE_KEXT_OVERRIDE;
//    bool exitVPC() APPLE_KEXT_OVERRIDE;

    static constexpr const char *getECStatus          = "ECAV";

    /**
     *  Initialize VPC EC status
     *
     *  @return true if success
     */
    bool initEC();

public:
//    IOReturn message(UInt32 type, IOService *provider, void *argument) APPLE_KEXT_OVERRIDE;
};


#endif /* MateVPC_hpp */
