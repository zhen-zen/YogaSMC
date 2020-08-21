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
#define backlightPrompt "BacklightLevel"
#define beepPrompt "Beep"
#define SSTPrompt "SST"
#define conservationPrompt "ConservationMode"
#define clamshellPrompt "ClamshellMode"
#define DYTCPrompt "DYTCMode"
#define ECLockPrompt "ECLock"
#define FnKeyPrompt "FnlockMode"
#define fanControlPrompt "FanControl"
#define KeyboardPrompt "KeyboardMode"
#define HotKeyPrompt "HotKey"
#define LEDPrompt "LED"
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

enum DYTC_command {
    DYTC_CMD_VER   = 0,    /* Get DYTC Version */
    DYTC_CMD_SET   = 1,    /* Set current IC function and mode */
    DYTC_CMD_GET   = 2,    /* Get current IC function and mode */
    /* 3-7, 0x0100 unknown yet, capability? */
    DYTC_CMD_RESET = 0x1ff,    /* Reset current IC function and mode */
};

struct __attribute__((packed)) DYTC_ARG {
    UInt32 command;
    UInt8 ICFunc;
    UInt8 ICMode;
    UInt8 validF;
};

#define DYTC_GET_LAPMODE_BIT 17 /* Set when in lapmode */

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

    /* Dynamic thermal control, available on IdeaVPC, ThinkVPC */
    static constexpr const char *setThermalControl = "DYTC";

    /**
     *  Clamshell mode capability, will be update on init
     */
    bool clamshellCap {false};

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

    /**
     *  Iterate over IOACPIPlane for PNP device
     *
     *  @param id PNP name
     *  @param dev target ACPI device
     *  @return true on sucess
     */
    bool findPNP(const char *id, IOACPIPlatformDevice **dev);

    /**
     *  Related ACPI methods
     */
    static constexpr const char *readECOneByte      = "RE1B";
    static constexpr const char *readECBytes        = "RECB";
    static constexpr const char *writeECOneByte     = "WE1B";
    static constexpr const char *writeECBytes       = "WECB";

    /**
     *  Dump desired EC field
     *
     *  @param value = offset | size << 8
     */
    void dumpECOffset(UInt32 value);

protected:
    const char* name;

    IOWorkLoop *workLoop {nullptr};
    IOCommandGate *commandGate {nullptr};

    /**
     *  EC device
     */
    IOACPIPlatformDevice *ec {nullptr};

    /**
     *  VPC device
     */
    IOACPIPlatformDevice *vpc {nullptr};
    
    /**
     *  Initialize VPC EC, get config and update status
     *
     *  @return true if success
     */
    virtual bool initVPC();

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

    /**
     *  Automatically turn off backlight mode on sleep
     *  BIT 0 sleep
     *  BIT 1 Yoga Mode
     */
    int automaticBacklightMode {3};

    /**
     *  Backlight mode capability, will be update on init
     */
    bool backlightCap {false};

    /**
     *  Backlight level
     */
    UInt32 backlightLevel {0};

    /**
     *  Backlight level before sleep or yoga mode change
     */
    UInt32 backlightLevelSaved {0};

    /**
     *  Update keyboad backlight status
     *
     *  @param update only update internal status when false
     *
     *  @return true if success
     */
    inline virtual bool updateBacklight(bool update=true) {return true;};

    /**
     *  Set backlight mode
     *
     *  @return true if success
     */
    inline virtual bool setBacklight(UInt32 level) {return true;};

    /**
     *  DYTC capability, will be update on init
     */
    bool DYTCCap {false};

    /**
     *  DYTC mode
     */
    UInt64 DYTCMode {0};

    /**
     *  Set DYTC mode
     *
     *  @param command  see DYTC_command
     *  @param result result
     *  @param ICFunc
     *  @param ICMode
     *  @param ValidF
     *  @param update  only update internal status when false
     *
     *  @return true if success
     */
    bool setDYTCMode(UInt32 command, UInt64* result, UInt8 ICFunc=0, UInt8 ICMode=0, bool ValidF=false, bool update=true);

    /**
     *  Wrapper for RE1B
     *
     *  @param offset EC field offset
     *  @param result EC field value
     *
     *  @return kIOReturnSuccess on success
     */
    IOReturn method_re1b(UInt32 offset, UInt32 *result);

    /**
     *  Wrapper for RECB
     *
     *  @param offset EC field offset
     *  @param size EC field length in bytes
     *  @param data EC field value
     *  @return kIOReturnSuccess on success
     */
    IOReturn method_recb(UInt32 offset, UInt32 size, UInt8 *data);

    /**
     *  Wrapper for WE1B
     *
     *  @param offset EC field offset
     *  @param value EC field value
     *
     *  @return kIOReturnSuccess on success
     */
    IOReturn method_we1b(UInt32 offset, UInt32 result);

    /**
     *  Read custom field
     *
     *  @param name EC field name
     *  @param result EC field value
     *
     *  @return kIOReturnSuccess on success
     */
    IOReturn readECName(const char* name, UInt32 *result);

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
