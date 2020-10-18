//  SPDX-License-Identifier: GPL-2.0-only
//
//  YogaWMI.hpp
//  YogaSMC
//
//  Created by Zhen on 2020/5/21.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "WMI.h"
#include "YogaBaseService.hpp"

class YogaWMI : public YogaBaseService
{
    typedef YogaBaseService super;
    OSDeclareDefaultStructors(YogaWMI)

protected:
    /**
     *  WMI device, in place of provider and direct ACPI evaluations
     */
    WMI* YWMI {nullptr};

    /**
     *  Find notify id and other properties of an event
     *
     *  @param key  name of event dictionary
     */
    void getNotifyID(OSString *key);

    /**
     *  Get pnp id of VPC device
     *
     *  @return nullptr if not specified
     */
    inline virtual const char *getVPCName() {return nullptr;};

    /**
     *  Vendor specific WMI analyze
     */
    inline virtual void processWMI() {};

    OSDictionary *Event {nullptr};

    /**
     *  Check WMI event id
     *
     *  @param cname WMI event name
     *  @param id  WMI event id
     */
    virtual void checkEvent(const char *cname, UInt32 id);
    
    /**
     *  Corresponding event to trigger after receiving a message
     *
     *  @param argument  argument of message
     */
    virtual void ACPIEvent(UInt32 argument);

public:
    virtual IOService *probe(IOService *provider, SInt32 *score) APPLE_KEXT_OVERRIDE;

    virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
    virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;

    virtual IOReturn message(UInt32 type, IOService *provider, void *argument) APPLE_KEXT_OVERRIDE;
    virtual IOReturn setPowerState(unsigned long powerState, IOService * whatDevice) APPLE_KEXT_OVERRIDE;
};
