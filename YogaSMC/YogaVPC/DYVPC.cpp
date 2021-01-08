//
//  DYVPC.cpp
//  YogaSMC
//
//  Created by Zhen on 1/7/21.
//  Copyright © 2021 Zhen. All rights reserved.
//

#include "DYVPC.hpp"
OSDefineMetaClassAndStructors(DYVPC, YogaVPC);

bool DYVPC::probeVPC(IOService *provider) {
    YWMI = new WMI(provider);
    YWMI->initialize();
    inputCap = YWMI->hasMethod(INPUT_WMI_EVENT, ACPI_WMI_EVENT);
    BIOSCap = YWMI->hasMethod(BIOS_QUERY_WMI_METHOD);
    if (!inputCap && !BIOSCap) {
        delete YWMI;
        return false;
    }
    return true;
}

bool DYVPC::initVPC() {
    if (inputCap)
        YWMI->enableEvent(INPUT_WMI_EVENT, true);

    return true;
}

bool DYVPC::exitVPC() {
    if (inputCap)
        YWMI->enableEvent(INPUT_WMI_EVENT, false);

    if (YWMI)
        delete YWMI;

    return super::exitVPC();
}

void DYVPC::updateVPC() {
    UInt32 id;
    UInt32 data;

    OSObject *result;
    if (!YWMI->getEventData(kIOACPIMessageReserved, &result))
        return;

    OSData *buf = OSDynamicCast(OSData, result);
    if (!buf) {
        DebugLog("Unknown response type");
        result->release();
        return;
    }

    switch (buf->getLength()) {
        case 8:
            id = *(reinterpret_cast<UInt32 const*>(buf->getBytesNoCopy(0, 4)));
            data = *(reinterpret_cast<UInt32 const*>(buf->getBytesNoCopy(4, 4)));
            break;
            
        case 16:
            id = *(reinterpret_cast<UInt32 const*>(buf->getBytesNoCopy(0, 8)));
            data = *(reinterpret_cast<UInt32 const*>(buf->getBytesNoCopy(8, 8)));
            break;
            
        default:
            DebugLog("Unknown response length %d", buf->getLength());
            buf->release();
            return;
    }
    buf->release();

    DebugLog("Unknown id: %d - 0x%x", id, data);
}

IOReturn DYVPC::message(UInt32 type, IOService *provider, void *argument) {
    switch (type)
    {
        case kSMC_setDisableTouchpad:
        case kSMC_getDisableTouchpad:
        case kPS2M_notifyKeyPressed:
        case kPS2M_notifyKeyTime:
        case kPS2M_resetTouchpad:
        case kSMC_setKeyboardStatus:
        case kSMC_getKeyboardStatus:
        case kSMC_notifyKeystroke:
            break;

        case kIOACPIMessageDeviceNotification:
            if (!argument)
                AlwaysLog("message: Unknown ACPI notification");
            else if (*((UInt32 *) argument) == kIOACPIMessageReserved)
                updateVPC();
            else
                AlwaysLog("message: Unknown ACPI notification 0x%04x", *((UInt32 *) argument));
            break;

        default:
            if (argument)
                AlwaysLog("message: type=%x, provider=%s, argument=0x%04x", type, provider->getName(), *((UInt32 *) argument));
            else
                AlwaysLog("message: type=%x, provider=%s", type, provider->getName());
    }

    return kIOReturnSuccess;
}
