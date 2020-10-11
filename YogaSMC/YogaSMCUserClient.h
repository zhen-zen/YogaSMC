//
//  YogaSMCUserClient.h
//  YogaSMC
//
//  Created by Zhen on 10/9/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef YogaSMCUserClient_h
#define YogaSMCUserClient_h

#ifndef _OS_OSTYPES_H

typedef unsigned int       UInt;
typedef signed int         SInt;

typedef unsigned char      UInt8;
typedef unsigned short     UInt16;
#if __LP64__
typedef unsigned int       UInt32;
#else
typedef unsigned long      UInt32;
#endif
typedef unsigned long long UInt64;

typedef signed char        SInt8;
typedef signed short       SInt16;
#if __LP64__
typedef signed int         SInt32;
#else
typedef signed long        SInt32;
#endif
typedef signed long long   SInt64;

#endif

#define kVPCUCBufSize    32

enum {
    kYSMCUCOpen,            // ScalarIScalarO
    kYSMCUCClose,        // ScalarIScalarO
    kYSMCUCRead,            // StructIStructO
    kYSMCUCReadEC,        // ScalarIStructO
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

#endif /* YogaSMCUserClient_h */
