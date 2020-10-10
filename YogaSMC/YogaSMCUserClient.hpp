//
//  YogaSMCUserClient.hpp
//  YogaSMC
//
//  Created by Zhen on 10/7/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef YogaSMCUserClient_hpp
#define YogaSMCUserClient_hpp

#include <IOKit/IOUserClient.h>
#include "common.h"
#include "YogaVPC.hpp"

#define kVPCUCBufSize    32

enum {
    kYSMCUCOpen,            // ScalarIScalarO
    kYSMCUCClose,        // ScalarIScalarO
    kYSMCUCRead,            // StructIStructO
    kYSMCUCWrite,        // StructIStructO
    kYSMCUCNotify,            // StructIStructO
    kYSMCUCNumMethods    
};

typedef struct {
    UInt8 mode;
    UInt8 addr;
    UInt16 count;
} VPCReadInput;

typedef struct {
    UInt16 count;
    UInt8 buf[kVPCUCBufSize];
} VPCReadOutput;

enum {
    kVPCUCDumbMode,
    kVPCUCRE1B,
    kVPCUCRECB,
};

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
//    IOReturn write( void *inStruct, void *outStruct, void *inCount, void *outCount, void *p5, void *p6 );
//    IOReturn notify( void *inStruct, void *inCount, void *p3, void *p4, void *p5, void *p6 );

};
#endif /* YogaSMCUserClient_hpp */
