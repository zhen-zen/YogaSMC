//
//  YogaSMCUserClient.cpp
//  YogaSMC
//
//  Created by Zhen on 10/7/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include <IOKit/IOLib.h>
#include "YogaSMCUserClient.h"
#include "YogaSMCUserClientPrivate.hpp"

OSDefineMetaClassAndStructors(YogaSMCUserClient, IOUserClient)
// https://github.com/HelmutJ/CocoaSampleCode/blob/master/SimpleUserClient/SimpleUserClient.cpp
IOExternalMethod *
YogaSMCUserClient::getTargetAndMethodForIndex(IOService **target, UInt32 index)
{
    static const IOExternalMethod sMethods[kYSMCUCNumMethods] =
    {
        {    // kYSMCUCOpen
            NULL,    // IOService * determined at runtime below
            (IOMethod) &YogaSMCUserClient::userClientOpen,
            kIOUCScalarIScalarO,
            0,    // no inputs
            0    // no outputs
        },
        {    // kYSMCUCClose
            NULL,    // IOService * determined at runtime below
            (IOMethod) &YogaSMCUserClient::userClientClose,
            kIOUCScalarIScalarO,
            0,    // no inputs
            0    // no outputs
        },
        {    // kYSMCUCRead
            NULL,    // IOService * determined at runtime below
            (IOMethod) &YogaSMCUserClient::read,
            kIOUCStructIStructO,
            sizeof(VPCReadInput),
            sizeof(VPCReadOutput)
        },
        {    // kYSMCUCReadEC
            NULL,    // IOService * determined at runtime below
            (IOMethod) &YogaSMCUserClient::readEC,
            kIOUCScalarIStructO,
            1,
            kIOUCVariableStructureSize
        },
//        {    // kYSMCUCWrite
//            NULL,    // IOService * determined at runtime below
//            (IOMethod) &YogaSMCUserClient::write,
//            kIOUCStructIStructO,
//            0,
//            0
//        },
//        {    // kYSMCUCNotify
//            NULL,    // IOService * determined at runtime below
//            (IOMethod) &YogaSMCUserClient::notify,
//            kIOUCStructIStructO,
//            0,
//            0
//        }
    };

    if (index < (UInt32)kYSMCUCWrite) {
        DebugLog("%s (index=%u)", __FUNCTION__, index);
        *target = this;
        return((IOExternalMethod *) &sMethods[index]);
    } else {
        AlwaysLog("%s not found (index=%u)", __FUNCTION__, index);
        *target = NULL;
        return(NULL);
    }
}

IOReturn YogaSMCUserClient::userClientOpen(void) {
    if (fProvider == NULL || isInactive()) {
        AlwaysLog("%s kIOReturnNotAttached", __FUNCTION__);
        return kIOReturnNotAttached;
    } else if (!fProvider->open(this)) {
        AlwaysLog("%s kIOReturnExclusiveAccess", __FUNCTION__);
        return kIOReturnExclusiveAccess;
    }
    DebugLog("%s", __FUNCTION__);
    return kIOReturnSuccess;
}

IOReturn YogaSMCUserClient::userClientClose(void) {
    if (fProvider == NULL || isInactive()) {
        AlwaysLog("%s kIOReturnNotAttached", __FUNCTION__);
        return kIOReturnNotAttached;
    } else if (!fProvider->isOpen(this)) {
        DebugLog("%s kIOReturnNotOpen", __FUNCTION__);
        return kIOReturnNotOpen;
    }
    fProvider->close(this);
    DebugLog("%s", __FUNCTION__);
    return kIOReturnSuccess;
}

IOReturn YogaSMCUserClient::read(void *inStruct, void *outStruct, void *inCount, void *outCount, void *p5, void *p6) {
    IOByteCount inputSize, *outputSizeP;
    VPCReadInput *input;
    VPCReadOutput *output;

    if (!fProvider->isOpen(this))
        return kIOReturnNotReady;

    input        = (VPCReadInput *)inStruct;
    inputSize    = (IOByteCount)inCount;
    output       = (VPCReadOutput *)outStruct;
    outputSizeP  = (IOByteCount *)outCount;

    if (!(fProvider && input && inputSize && output && outputSizeP) ||
        (inputSize != sizeof(VPCReadInput)) ||
        (*outputSizeP != sizeof(VPCReadOutput)) ||
        (input->count > kVPCUCBufSize)) {
        AlwaysLog("%s invalid arguments", __FUNCTION__);
        return kIOReturnBadArgument;
    }

    switch (input->mode) {
        case kVPCUCDumbMode:
            DebugLog("%s kVPCUCDumbMode", __FUNCTION__);
            break;
        case kVPCUCRE1B:
            DebugLog("%s kVPCUCRE1B", __FUNCTION__);
            if (input->count != 1)
                return kIOReturnBadArgument;
            if (fProvider->method_re1b(input->addr, (UInt32 *)(output->buf)) != kIOReturnSuccess)
                return kIOReturnIOError;
            output->count = 1;
            break;

        case kVPCUCRECB:
            DebugLog("%s kVPCUCRECB", __FUNCTION__);
            if (input->count > kVPCUCBufSize)
                return kIOReturnBadArgument;
            OSData *result;
            if (fProvider->method_recb(input->addr, input->count, &result) != kIOReturnSuccess)
                return kIOReturnIOError;
            if (result->getLength() == input->count) {
                const UInt8* data = reinterpret_cast<const UInt8 *>(result->getBytesNoCopy());
                memcpy(output->buf, data, result->getLength());
            }
            output->count = result->getLength();
            result->release();
            break;

        default:
            AlwaysLog("%s invalid mode", __FUNCTION__);
            return kIOReturnBadArgument;
            break;
    }
    return kIOReturnSuccess;
}

IOReturn YogaSMCUserClient::readEC(UInt64 offset, UInt8* output, IOByteCount *outputSizeP) {
    if (!fProvider->isOpen(this))
        return kIOReturnNotReady;

    if (!(fProvider && output && outputSizeP)) {
        AlwaysLog("%s invalid arguments", __FUNCTION__);
        return kIOReturnBadArgument;
    }

    if (offset + *outputSizeP > 0x100) {
        AlwaysLog("%s invalid range", __FUNCTION__);
        return kIOReturnBadArgument;
    }

    DebugLog("%s", __FUNCTION__);

    if (*outputSizeP == 1) {
        UInt32 result;
        if (fProvider->method_re1b(UInt32(offset), &result) != kIOReturnSuccess)
            return kIOReturnIOError;
        *output = UInt8(result);
    } else {
        OSData *result;
        if (fProvider->method_recb(UInt32(offset), UInt32(*outputSizeP), &result) != kIOReturnSuccess)
            return kIOReturnIOError;
        if (result->getLength() == *outputSizeP) {
            const UInt8* data = reinterpret_cast<const UInt8 *>(result->getBytesNoCopy());
            memcpy(output, data, result->getLength());
        } else {
            *outputSizeP = 0;
        }
        result->release();
    }
    return kIOReturnSuccess;
}

bool YogaSMCUserClient::start(IOService *provider) {
    if (!(fProvider = OSDynamicCast(YogaVPC, provider)))
        return false;
    name = fProvider->getName();
    DebugLog("%s", __FUNCTION__);
    return super::start(provider);
}

void YogaSMCUserClient::stop(IOService *provider) {
    DebugLog("%s", __FUNCTION__);
    super::stop(provider);
}

IOReturn YogaSMCUserClient::clientClose(void) {
    userClientClose();
    if (terminate())
        DebugLog("%s success", __FUNCTION__);
    else
        AlwaysLog("%s failed", __FUNCTION__);
    return kIOReturnSuccess;
}

IOReturn YogaSMCUserClient::clientDied(void) {
    AlwaysLog("%s", __FUNCTION__);
    return super::clientDied();
}

