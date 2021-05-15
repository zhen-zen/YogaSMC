//  SPDX-License-Identifier: GPL-2.0-only
//
//  IdeaSMC.hpp
//  YogaSMC
//
//  Created by Zhen on 8/25/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef IdeaSMC_hpp
#define IdeaSMC_hpp

#include "IdeaWMI.hpp"
#include "YogaSMC.hpp"

class IdeaSMC : public YogaSMC
{
    typedef YogaSMC super;
    OSDeclareDefaultStructors(IdeaSMC)

private:
    /**
     *  WMI device, in place of provider and direct ACPI evaluations
     */
    IdeaWMIGameZone *wmig {nullptr};

    /**
     *  Corresponding sensor index
     */
    UInt8 sensorIndex[MAX_SENSOR];

    void addVSMCKey() APPLE_KEXT_OVERRIDE;
    void updateEC() APPLE_KEXT_OVERRIDE;
    virtual inline IOTimerEventSource *initPoller() APPLE_KEXT_OVERRIDE {
        return IOTimerEventSource::timerEventSource(this, [](OSObject *object, IOTimerEventSource *sender) {
            auto smc = OSDynamicCast(IdeaSMC, object);
            if (smc) smc->updateEC();
        });
    };

public:
    static IdeaSMC *withDevice(IOService *provider, IOACPIPlatformDevice *device);

    void setWMI(IOService* instance);
};

#endif /* IdeaSMC_hpp */
