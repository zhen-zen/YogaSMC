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

class YogaSMCUserClient;
class YogaVPC : public YogaBaseService
{
  typedef YogaBaseService super;
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
     *  DYTC capability, will be update on init
     */
    UInt16 DYTCCap {0};

    /**
     *  Simple lock to prevent DYTC update when user setting is in progress
     */
    bool DYTCLock {false};
    
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
     *  Initialize DYTC property
     */
    void initDYTC();

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

    /**
     *  Probe compatible WMI devices
     */
    void probeWMI();

protected:

    /**
     *  VPC device
     */
    IOACPIPlatformDevice *vpc {nullptr};
    
    OSOrderedSet *WMICollection {nullptr};
    OSOrderedSet *WMIProvCollection {nullptr};

    /**
     *  Clamshell mode status, default to false (same in SSDT)
     */
    bool clamshellMode {false};

    /**
     *  Vendor specific WMI support
     */
    bool vendorWMISupport {false};

    /**
     *  Initialize WMI
     *
     *  @param instance WMI instance
     *
     *  @return Initialized YogaWMI instance
     */
    inline virtual IOService* initWMI(WMI *instance) {return nullptr;};

    /**
     *  Examine WMI
     *
     *  @param provider service provider
     *
     *  @return true if success
     */
    inline virtual bool examineWMI(IOService *provider) {return true;};

#ifndef ALTER
    /**
     *  SMC service
     */
    IOService *smc;

    /**
     *  Initialize SMC
     */
    virtual IOService* initSMC();
#endif
    /**
     *  Initialize VPC EC, get config and update status
     *
     *  @return true if success
     */
    virtual bool initVPC();

    /**
     *  Probe VPC
     *
     *  @param provider service provider
     *
     *  @return true if success
     */
    inline virtual bool probeVPC(IOService *provider) {return true;};

    /**
     *  Update VPC EC status
     *
     *  @param event Event ID
     */
    inline virtual void updateVPC(UInt32 event=0) {return;};

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
    UInt32 automaticBacklightMode {0x17};

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
    inline virtual bool updateBacklight(bool update=false) {return false;};

    /**
     *  Set backlight mode
     *
     *  @return true if success
     */
    inline virtual bool setBacklight(UInt32 level) {return true;};

    /**
     *  Notify battery on conservation mode change
     *
     *  @return true if success
     */
    bool notifyBattery();

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

    /**
     *  Wrapper for single param integer evaluation
     *
     *  @param objectName    ACPI method name
     *  @param resultInt32  Pointer to result, skipped if null
     *  @param paramInt32    Single integer param
     *
     *  @return kIOReturnSuccess if success
     */
    IOReturn evaluateIntegerParam(const char *objectName, UInt32 *resultInt32 , UInt32 paramInt32);

public:
    virtual IOService *probe(IOService *provider, SInt32 *score) APPLE_KEXT_OVERRIDE;

    virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
    virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;

    virtual IOReturn setProperties(OSObject* props) APPLE_KEXT_OVERRIDE;
    virtual IOReturn message(UInt32 type, IOService *provider, void *argument) APPLE_KEXT_OVERRIDE;
    virtual IOReturn setPowerState(unsigned long powerState, IOService * whatDevice) APPLE_KEXT_OVERRIDE;

    /**
     *  VPC UserClient
     */
    YogaSMCUserClient *client {nullptr};
};

#endif /* YogaVPC_hpp */
