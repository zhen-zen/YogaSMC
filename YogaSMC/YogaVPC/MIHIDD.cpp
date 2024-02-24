//
//  MIHIDD.cpp
//  YogaSMC
//
//  Created by Zhen on 5/7/21.
//  Copyright © 2021 Zhen. All rights reserved.
//

#include "MIHIDD.hpp"

OSDefineMetaClassAndStructors(MIHIDD, YogaVPC);

bool MIHIDD::probeVPC(IOService *provider) {
    YWMI = new WMI(provider);
    if (!YWMI->initialize() || !YWMI->hasMethod(FS_SERVICE_WMI_METHOD, ACPI_WMI_EVENT)) {
        delete YWMI;
        return false;
    }
    return true;
}

bool MIHIDD::initVPC() {
    bool ret;
    UInt32 status;
    OSObject* params[3] = {
        OSNumber::withNumber(0ULL, 32),
        OSNumber::withNumber(FS_SERVICE_WMI_STATUS, 32),
        OSNumber::withNumber(0ULL, 32)
    };
    
    ret = YWMI->executeInteger(FS_SERVICE_WMI_METHOD, &status, params, 3);
    params[0]->release();
    params[1]->release();
    params[2]->release();

    if (ret)
        setProperty("IEFS_Status", status, 32);

    OSDictionary *Event = YWMI->getEvent();
    if (Event) {
        setProperty("Event", Event);
        OSCollectionIterator* i = OSCollectionIterator::withCollection(Event);

        if (i) {
            while (OSString* key = OSDynamicCast(OSString, i->getNextObject()))
                YWMI->enableEvent(key->getCStringNoCopy(), true);
            i->release();
        }
    }
    return true;
}
