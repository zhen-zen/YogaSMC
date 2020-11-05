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

#include "DYTC.h"
#include "YogaBaseService.hpp"
#include "YogaSMCUserClientPrivate.hpp"

#ifndef ALTER
#include "YogaSMC.hpp"
#endif

class YogaSMCUserClient;
class YogaVPC : public YogaBaseService
{
  typedef YogaBaseService super;
  OSDeclareDefaultStructors(YogaVPC);

private:
    inline virtual bool PMSupport() APPLE_KEXT_OVERRIDE {return true;};

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
     *  Dump desired EC field
     *
     *  @param value = offset | size << 8
     *
     *  @return true if success
     */
    bool dumpECOffset(UInt32 value);

    
    /**
     *  Set DYTC mode
     *
     *  @param command  see DYTC_command
     *  @param result result
     *  @param ICFunc function
     *  @param ICMode mode
     *  @param ValidF on / off
     *
     *  @return true if success
     */
    bool DYTCCommand(DYTC_CMD command, DYTC_RESULT* result, UInt8 ICFunc=0, UInt8 ICMode=0, bool ValidF=false);

    /**
     *  Parse DYTC status
     *
     *  @param result result from DYTC command
     *
     *  @return true if success
     */
    bool parseDYTC(DYTC_RESULT result);
    
    /**
     *  DYTC Version
     */
    OSDictionary *DYTCVersion {nullptr};

protected:

    /**
     *  VPC device
     */
    IOACPIPlatformDevice *vpc {nullptr};
    
    OSOrderedSet *WMICollection {nullptr};
    OSOrderedSet *WMIProvCollection {nullptr};

    /**
     *  Initialize WMI
     *
     *  @param provider WMI provider
     *
     *  @return Initialized YogaWMI instance
     */
    inline virtual IOService* initWMI(IOACPIPlatformDevice *provider) {return nullptr;};

#ifndef ALTER
    /**
     *  SMC service
     */
    YogaSMC *smc;

    /**
     *  Initialize SMC
     */
    inline virtual void initSMC() {
        smc = YogaSMC::withDevice(this, ec);
        smc->conf = OSDynamicCast(OSDictionary, getProperty("Sensors"));
    };
#endif
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
     *  BIT 0 Keyboard backlight
     *  BIT 1 Keyboard backlight on yoga mode
     *  BIT 2 _SI._SST
     *  BIT 3 Mute LED
     *  BIT 4 Mic Mute LED
     */
    UInt32 automaticBacklightMode {0x1f};

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
     *  @param update true if triggered from hardware
     *
     *  @return true if success
     */
    inline virtual bool updateBacklight(bool update=false) {return true;};

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
     *  DYTC lapmode capability, will be update on init
     */
    bool DYTCLapmodeCap {false};

    /**
     *  Simple lock to prevent DYTC update when user setting is in progress
     */
    bool DYTCLock {false};

    /**
     *  Update DYTC status
     *
     *  @return true if success
     */
    bool updateDYTC();

    /**
     *  Set DYTC status
     *
     *  @param perfmode Performance mode
     *
     *  @return true if success
     */
    bool setDYTC(int perfmode);

public:
    virtual IOService *probe(IOService *provider, SInt32 *score) APPLE_KEXT_OVERRIDE;

    virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
    virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;

    virtual IOReturn setProperties(OSObject* props) APPLE_KEXT_OVERRIDE;
    virtual IOReturn setPowerState(unsigned long powerState, IOService * whatDevice) APPLE_KEXT_OVERRIDE;

    /**
     *  VPC UserClient
     */
    YogaSMCUserClient *client {nullptr};
};

#endif /* YogaVPC_hpp */
