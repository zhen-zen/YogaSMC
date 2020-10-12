//
//  YogaSMCUserClient.h
//  YogaSMC
//
//  Created by Zhen on 10/9/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef YogaSMCUserClient_h
#define YogaSMCUserClient_h

#include <libkern/OSTypes.h>
#include <mach/message.h>

#ifndef IOKIT
typedef uint64_t io_user_reference_t;
#endif

#define kVPCUCBufSize    32

enum {
    kYSMCUCOpen,            // ScalarIScalarO
    kYSMCUCClose,        // ScalarIScalarO
    kYSMCUCReadEC,        // ScalarIStructO
    kYSMCUCWriteEC,        // ScalarIStructI
    kYSMCUCNumMethods
};

typedef struct {
    mach_msg_header_t header;
    UInt32 event;
    io_user_reference_t ref;
} SMCNotificationMessage;

#endif /* YogaSMCUserClient_h */
