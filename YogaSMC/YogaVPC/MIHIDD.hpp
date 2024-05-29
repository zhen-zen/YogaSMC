//
//  MIHIDD.hpp
//  YogaSMC
//
//  Created by Zhen on 5/7/21.
//  Copyright © 2021 Zhen. All rights reserved.
//

#ifndef MIHIDD_hpp
#define MIHIDD_hpp

#include "YogaVPC.hpp"

#define FS_SERVICE_WMI_METHOD "d8ab598e-16a5-472f-b9a4-9b9a4ae782a6"

#define FS_SERVICE_WMI_STATUS 2

class MIHIDD : public YogaVPC
{
    typedef YogaVPC super;
    OSDeclareDefaultStructors(MIHIDD)

    bool probeVPC(IOService *provider) APPLE_KEXT_OVERRIDE;
    bool initVPC() APPLE_KEXT_OVERRIDE;
//    void updateVPC(UInt32 event) APPLE_KEXT_OVERRIDE;
//    bool exitVPC() APPLE_KEXT_OVERRIDE;
//public:
//    IOReturn message(UInt32 type, IOService *provider, void *argument) APPLE_KEXT_OVERRIDE;
};
#endif /* MIHIDD_hpp */
