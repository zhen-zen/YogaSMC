//  SPDX-License-Identifier: GPL-2.0-only
//
//  YogaVPC.hpp
//  YogaSMC
//
//  Created by Zhen on 2020/7/10.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef YogaVPC_hpp
#define YogaVPC_hpp

#include <IOKit/IOCommandGate.h>
#include <IOKit/IOService.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>

#define batteryPrompt "Battery"
#define conservationPrompt "ConservationMode"
#define readECPrompt "ReadEC"
#define writeECPrompt "WriteEC"
#define FnKeyPrompt "FnlockMode"

#define PnpDeviceIdVPCIdea "VPC2004"
#define PnpDeviceIdVPCThink "LEN0268"

class YogaVPC : public IOService
{
  typedef IOService super;
  OSDeclareDefaultStructors(YogaVPC);

protected:
    IOWorkLoop *workLoop {nullptr};
    IOCommandGate *commandGate {nullptr};

    /**
     *  VPC device
     */
    IOACPIPlatformDevice *vpc {nullptr};
    
    inline virtual bool initVPC() {return true;};

    /**
     *  Update VPC EC status
     */
    inline virtual void updateVPC() {return;};

    inline virtual void setPropertiesGated(OSObject* props) {return;};

public:
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;

    static YogaVPC* withDevice(IOACPIPlatformDevice *device, OSString *pnp);
    IOReturn setProperties(OSObject* props) override;
    IOReturn message(UInt32 type, IOService *provider, void *argument) override;
};

#endif /* YogaVPC_hpp */
