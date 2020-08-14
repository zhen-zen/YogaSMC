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
#include "message.h"

#define autoBacklightPrompt "AutoBacklight"
#define batteryPrompt "Battery"
#define backlightPrompt "BacklightMode"
#define conservationPrompt "ConservationMode"
#define clamshellPrompt "ClamshellMode"
#define ECLockPrompt "ECLock"
#define FnKeyPrompt "FnlockMode"
#define fanControlPrompt "FanControl"
#define KeyboardPrompt "KeyboardMode"
#define HotKeyPrompt "HotKey"
#define mutePrompt "Mute"
#define muteLEDPrompt "MuteLED"
#define muteSupportPrompt "MuteSupport"
#define micMuteLEDPrompt "MicMuteLED"
#define VPCPrompt "VPCconfig"
#define rapidChargePrompt "RapidChargeMode"
#define readECPrompt "ReadEC"
#define resetPrompt "reset"
#define writeECPrompt "WriteEC"
#define updatePrompt "Update"

#define initFailure "%s::%s %s evaluation failed, exiting\n"
#define updateFailure "%s::%s %s evaluation failed\n"
#define updateSuccess "%s::%s %s 0x%x\n"
#define toggleFailure "%s::%s %s toggle failed\n"
#define toggleSuccess "%s::%s %s set to 0x%x: %s\n"

#define valueMatched "%s::%s %s already %x\n"
#define valueInvalid "%s::%s Invalid value for %s\n"

#define timeoutPrompt "%s::%s %s timeout 0x%x\n"
#define VPCUnavailable "%s::%s VPC unavailable\n"

#define BIT(nr) (1U << (nr))

class YogaVPC : public IOService
{
  typedef IOService super;
  OSDeclareDefaultStructors(YogaVPC);

private:
    /**
     *  Related ACPI methods
     *  See SSDTexamples
     */
    static constexpr const char *getClamshellMode  = "GCSM";
    static constexpr const char *setClamshellMode  = "SCSM";

    /**
     *  Clamshell mode status, default to false (same in SSDT)
     */
    bool clamshellMode {false};

    /**
     *  Update clamshell mode status
     *
     *  @param update  only update internal status when false
     *
     *  @return true if success
     */
    bool updateClamshell(bool update=true);

    /**
     *  Toggle clamshell mode
     *
     *  @return true if success
     */
    bool toggleClamshell();

protected:
    const char* name;

    IOWorkLoop *workLoop {nullptr};
    IOCommandGate *commandGate {nullptr};

    /**
     *  VPC device
     */
    IOACPIPlatformDevice *vpc {nullptr};
    
    /**
     *  Initialize VPC EC, get config and update status
     *
     *  @return true if success
     */
    inline virtual bool initVPC() {return true;};

    /**
     *  Update VPC EC status
     */
    inline virtual void updateVPC() {return;};

    /**
     *  Update all status
     */
    virtual void updateAll();

    /**
     *  Restore VPC EC status
     */
    virtual bool exitVPC();
    
    virtual void setPropertiesGated(OSObject* props);

public:
    virtual bool init(OSDictionary *dictionary) APPLE_KEXT_OVERRIDE;
    virtual IOService *probe(IOService *provider, SInt32 *score) APPLE_KEXT_OVERRIDE;

    virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
    virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;

    static YogaVPC* withDevice(IOACPIPlatformDevice *device, OSString *pnp);
    virtual IOReturn setProperties(OSObject* props) APPLE_KEXT_OVERRIDE;
    virtual IOReturn message(UInt32 type, IOService *provider, void *argument) APPLE_KEXT_OVERRIDE;
    virtual IOReturn setPowerState(unsigned long powerState, IOService * whatDevice) APPLE_KEXT_OVERRIDE;
};

#endif /* YogaVPC_hpp */
