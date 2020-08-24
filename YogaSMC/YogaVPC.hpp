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
#include "common.h"
#include "message.h"

#ifndef ALTER
#include "YogaSMC.hpp"
#endif

enum DYTC_command {
    DYTC_CMD_QUERY = 0,    /* Get DYTC Version */
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

#define DYTC_QUERY_ENABLE_BIT 8  /* Bit 8 - 0 = disabled, 1 = enabled */
#define DYTC_QUERY_SUBREV_BIT 16 /* Bits 16 - 27 - sub revisision */
#define DYTC_QUERY_REV_BIT    28 /* Bits 28 - 31 - revision */

#define DYTC_GET_FUNCTION_BIT 8  /* Bits 8-11 - function setting */
#define DYTC_GET_MODE_BIT     12 /* Bits 12-15 - mode setting */
#define DYTC_GET_LAPMODE_BIT  17 /* Bit 17 - lapmode. Set when on lap */

#define DYTC_SET_FUNCTION_BIT 12 /* Bits 12-15 - funct setting */
#define DYTC_SET_MODE_BIT     16 /* Bits 16-19 - mode setting */
#define DYTC_SET_VALID_BIT    20 /* Bit 20 - 1 = on, 0 = off */

#define DYTC_FUNCTION_STD     0  /* Function = 0, standard mode */
#define DYTC_FUNCTION_CQL     1  /* Function = 1, lap mode */
#define DYTC_FUNCTION_MMC     11 /* Function = 11, desk mode */

#define DYTC_MODE_PERFORM     2  /* High power mode aka performance */
#define DYTC_MODE_QUIET       3  /* low power mode aka quiet */
#define DYTC_MODE_BALANCE   0xF  /* default mode aka balance */

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
     *
     *  @return true if success
     */
    bool dumpECOffset(UInt32 value);

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
#ifndef ALTER
    /**
     *  SMC service
     */
    YogaSMC *smc;

    /**
     *  Initialize SMC
     *
     *  @return true if success
     */
    inline virtual void initSMC() {smc = YogaSMC::withDevice(this, ec);};
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
     *  DYTC Revision
     */
    int DYTCRevision {0};

    /**
     *  DYTC Subversion
     */
    int DYTCSubRevision {0};

    /**
     *  Set DYTC mode
     *
     *  @param command  see DYTC_command
     *  @param result result
     *  @param ICFunc
     *  @param ICMode
     *  @param ValidF
     *
     *  @return true if success
     */
    bool DYTCCommand(UInt32 command, UInt64* result, UInt8 ICFunc=0, UInt8 ICMode=0, bool ValidF=false);

    /**
     *  Update DYTC status
     *
     *  @param update only update internal status when false
     *
     *  @return true if success
     */
    bool updateDYTC(bool update=true);

    /**
     *  Set DYTC status
     *
     *  @param perfmode Performance mode
     *
     *  @return true if success
     */
    bool setDYTC(int perfmode);

    /**
     *  EC read capability, will be update on init
     */
    bool ECReadCap {false};

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
     *
     *  @return kIOReturnSuccess on success
     */
    IOReturn method_recb(UInt32 offset, UInt32 size, OSData **data);

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

    virtual IOReturn setProperties(OSObject* props) APPLE_KEXT_OVERRIDE;
    virtual IOReturn message(UInt32 type, IOService *provider, void *argument) APPLE_KEXT_OVERRIDE;
    virtual IOReturn setPowerState(unsigned long powerState, IOService * whatDevice) APPLE_KEXT_OVERRIDE;
};

#endif /* YogaVPC_hpp */
