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
        {    // kYSMCUCWriteEC
            NULL,    // IOService * determined at runtime below
            (IOMethod) &YogaSMCUserClient::writeEC,
            kIOUCScalarIStructI,
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
    AlwaysLog("%s temporary deprecated", __FUNCTION__);
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

    if (*outputSizeP == 1) {
        UInt32 result;
        if (fProvider->method_re1b(UInt32(offset), &result) != kIOReturnSuccess) {
            AlwaysLog("%s re1b failed", __FUNCTION__);
            return kIOReturnIOError;
        }
        *output = UInt8(result);
    } else {
        OSData *result;
        if (fProvider->method_recb(UInt32(offset), UInt32(*outputSizeP), &result) != kIOReturnSuccess) {
            AlwaysLog("%s recb failed", __FUNCTION__);
            return kIOReturnIOError;
        }
        if (result->getLength() == *outputSizeP) {
            const UInt8* data = reinterpret_cast<const UInt8 *>(result->getBytesNoCopy());
            memcpy(output, data, result->getLength());
        } else {
            *outputSizeP = 0;
        }
        result->release();
    }
    DebugLog("%s", __FUNCTION__);
    return kIOReturnSuccess;
}

IOReturn YogaSMCUserClient::writeEC(UInt64 offset, UInt8* input, IOByteCount *inputSizeP) {
    if (!fProvider->isOpen(this))
        return kIOReturnNotReady;

    if (!(fProvider && input && inputSizeP)) {
        AlwaysLog("%s invalid arguments", __FUNCTION__);
        return kIOReturnBadArgument;
    }

    if (offset + *inputSizeP > 0x100) {
        AlwaysLog("%s invalid range", __FUNCTION__);
        return kIOReturnBadArgument;
    }

    if (*inputSizeP != 1) {
        AlwaysLog("%s bulk write is not supported yet", __FUNCTION__);
    } else {
        if (fProvider->method_we1b(UInt32(offset), UInt32(input[0])) != kIOReturnSuccess) {
            AlwaysLog("%s we1b failed", __FUNCTION__);
            return kIOReturnIOError;
        }
    }
    DebugLog("%s", __FUNCTION__);
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

