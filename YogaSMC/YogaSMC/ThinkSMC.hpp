//  SPDX-License-Identifier: GPL-2.0-only
//
//  ThinkSMC.hpp
//  YogaSMC
//
//  Created by Zhen on 8/30/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef ThinkSMC_hpp
#define ThinkSMC_hpp

#include "YogaSMC.hpp"

class ThinkSMC : public YogaSMC
{
    typedef YogaSMC super;
    OSDeclareDefaultStructors(ThinkSMC)

private:
    void updateECFan();
    void addVSMCKey() APPLE_KEXT_OVERRIDE;
    void updateEC() APPLE_KEXT_OVERRIDE;
    virtual inline IOTimerEventSource *initPoller() APPLE_KEXT_OVERRIDE {
        return IOTimerEventSource::timerEventSource(this, [](OSObject *object, IOTimerEventSource *sender) {
            auto smc = OSDynamicCast(ThinkSMC, object);
            if (smc) smc->updateEC();
        });
    };

    bool dualFan {false};

public:
    static ThinkSMC *withDevice(IOService *provider, IOACPIPlatformDevice *device);

};

#endif /* ThinkSMC_hpp */
