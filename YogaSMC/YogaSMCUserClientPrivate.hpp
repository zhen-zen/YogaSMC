//
//  YogaSMCUserClientPrivate.hpp
//  YogaSMC
//
//  Created by Zhen on 10/7/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef YogaSMCUserClientPrivate_hpp
#define YogaSMCUserClientPrivate_hpp

#include <IOKit/IOUserClient.h>
#include "common.h"
#include "YogaVPC.hpp"

class YogaSMCUserClient : public IOUserClient {
    typedef IOUserClient super;
    OSDeclareDefaultStructors(YogaSMCUserClient)

protected:
    YogaVPC* fProvider;
    const char* name;
    task_t fTask;

public:
    virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
    virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;

    virtual IOExternalMethod *getTargetAndMethodForIndex(IOService **target, UInt32 Index) APPLE_KEXT_OVERRIDE;

    virtual IOReturn clientClose(void) APPLE_KEXT_OVERRIDE;
    virtual IOReturn clientDied(void) APPLE_KEXT_OVERRIDE;

    // Externally accessible methods
    IOReturn userClientOpen(void);
    IOReturn userClientClose(void);
    IOReturn read( void *inStruct, void *outStruct, void *inCount, void *outCount, void *p5, void *p6 );
    IOReturn readEC(UInt64 offset, UInt8 *output, IOByteCount *outputSizeP);
    IOReturn writeEC(UInt64 offset, UInt8 *input, IOByteCount *inputSizeP);
//    IOReturn write( void *inStruct, void *outStruct, void *inCount, void *outCount, void *p5, void *p6 );
//    IOReturn notify( void *inStruct, void *inCount, void *p3, void *p4, void *p5, void *p6 );

};
#endif /* YogaSMCUserClientPrivate_hpp */
