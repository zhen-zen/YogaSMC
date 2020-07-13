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
#define clamshellPrompt "ClamshellMode"
#define FnKeyPrompt "FnlockMode"
#define VPCPrompt "VPCconfig"
#define readECPrompt "ReadEC"
#define writeECPrompt "WriteEC"
#define updatePrompt "Update"

#define updateFailure "%s: %s evaluation failed\n"
#define updateSuccess "%s: %s 0x%x\n"
#define toggleFailure "%s: %s toggle failed\n"
#define toggleSuccess "%s: %s set to 0x%x: %s\n"

#define valueMatched "%s: %s already %s\n"
#define valueInvalid "%s: Invalid value for %s\n"

#define timeoutPrompt "%s: %s timeout 0x%x\n"
#define VPCUnavailable "%s: VPC unavailable\n"

#define PnpDeviceIdVPCIdea "VPC2004"
#define PnpDeviceIdVPCThink "LEN0268"

class YogaVPC : public IOService
{
  typedef IOService super;
  OSDeclareDefaultStructors(YogaVPC);

private:
    /**
     *  Related ACPI methods
     */
    static constexpr const char *getClamshellMode  = "GCSM";
    static constexpr const char *setClamshellMode  = "SCSM";

    void updateAll();

    bool clamshellMode;

    bool updateClamshell(bool update=true);
    bool toggleClamshell();

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

    virtual void setPropertiesGated(OSObject* props);

public:
    virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
    virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;

    static YogaVPC* withDevice(IOACPIPlatformDevice *device, OSString *pnp);
    IOReturn setProperties(OSObject* props) APPLE_KEXT_OVERRIDE;
    IOReturn message(UInt32 type, IOService *provider, void *argument) APPLE_KEXT_OVERRIDE;
};

#endif /* YogaVPC_hpp */
