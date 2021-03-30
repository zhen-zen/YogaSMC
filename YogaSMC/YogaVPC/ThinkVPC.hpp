//  SPDX-License-Identifier: GPL-2.0-only
//
//  ThinkVPC.hpp
//  YogaSMC
//
//  Created by Zhen on 7/12/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef ThinkVPC_hpp
#define ThinkVPC_hpp

#include "YogaVPC.hpp"
#ifndef ALTER
#include "ThinkSMC.hpp"
#endif

// from linux/drivers/platform/x86/ideapad-laptop.c and ibm-acpi

#define KBD_BACKLIGHT_CAP_BIT (9)

#define DHKN_MASK_offset 0
#define DHKE_MASK_offset 0x20
#define DHKF_MASK_offset 0x40

enum tpacpi_mute_btn_mode {
    TP_EC_MUTE_BTN_LATCH  = 0,    /* Mute mutes; up/down unmutes */
    /* We don't know what mode 1 is. */
    TP_EC_MUTE_BTN_NONE   = 2,    /* Mute and up/down are just keys */
    TP_EC_MUTE_BTN_TOGGLE = 3,    /* Mute toggles; up/down unmutes */
};

enum  {
    TPACPI_AML_MUTE_GET_FUNC = 0x01,
    TPACPI_AML_MUTE_SET_FUNC = 0x02,
    TPACPI_AML_MUTE_SUPPORT_FUNC = 0x04,
    TPACPI_AML_MUTE_READ_MASK = 0x01,
    TPACPI_AML_MIC_MUTE_HDMC = 0x00010000,
    TPACPI_AML_MUTE_ERROR_STATE_MASK = 0x80000000,
};

enum {
    TP_ACPI_MULTI_MODE_INVALID    = 0,
    TP_ACPI_MULTI_MODE_UNKNOWN    = 1 << 0,
    TP_ACPI_MULTI_MODE_LAPTOP     = 1 << 1,
    TP_ACPI_MULTI_MODE_TABLET     = 1 << 2,
    TP_ACPI_MULTI_MODE_FLAT       = 1 << 3,
    TP_ACPI_MULTI_MODE_STAND      = 1 << 4,
    TP_ACPI_MULTI_MODE_TENT       = 1 << 5,
    TP_ACPI_MULTI_MODE_STAND_TENT = 1 << 6,
};

enum {
    /* The following modes are considered tablet mode for the purpose of
     * reporting the status to userspace. i.e. in all these modes it makes
     * sense to disable the laptop input devices such as touchpad and
     * keyboard.
     */
    TP_ACPI_MULTI_MODE_TABLET_LIKE    = TP_ACPI_MULTI_MODE_TABLET |
                                        TP_ACPI_MULTI_MODE_STAND |
                                        TP_ACPI_MULTI_MODE_TENT |
                                        TP_ACPI_MULTI_MODE_STAND_TENT,
};

enum {
    BAT_ANY = 0,
    BAT_PRIMARY = 1,
    BAT_SECONDARY = 2
};

class ThinkVPC : public YogaVPC
{
    typedef YogaVPC super;
    OSDeclareDefaultStructors(ThinkVPC)

private:
    /**
     *  Related ACPI methods
     */
    static constexpr const char *getHKEYversion        = "MHKV";
    static constexpr const char *getHKEYAdaptive       = "MHKA";
    static constexpr const char *getHKEYevent          = "MHKP";
//    static constexpr const char *getHKEYstatus         = "DHKC"; // Actually a NameObj, not a method
    static constexpr const char *setHKEYenable         = "MHKE"; // DHKB, higher layer, _PTS/_OWAK
    static constexpr const char *setHKEYstatus         = "MHKC"; // DHKC
    static constexpr const char *getHKEYmask           = "MHKN";
    static constexpr const char *setHKEYmask           = "MHKM";
    static constexpr const char *setHKEYBeep           = "MHKB";
    static constexpr const char *setHKEYsleep          = "MHKS"; // Trigger sleep button
    static constexpr const char *setHKEYlid            = "MHKD"; // Force lid close?

    static constexpr const char *setBeep               = "BEEP"; // EC
    static constexpr const char *setLED                = "LED"; // EC

    static constexpr const char *getCMstart            = "BCTG";
    static constexpr const char *setCMstart            = "BCCS";
    static constexpr const char *getCMstop             = "BCSG";
    static constexpr const char *setCMstop             = "BCSS";
    static constexpr const char *getCMInhibitCharge    = "BICG";
    static constexpr const char *setCMInhibitCharge    = "BICS";
    static constexpr const char *getCMForceDischarge   = "BDSG";
    static constexpr const char *setCMForceDischarge   = "BDSS";
    static constexpr const char *getCMPeakShiftState   = "PSSG";
    static constexpr const char *setCMPeakShiftState   = "PSSS";

    static constexpr const char *getKBDBacklightLevel  = "MLCG";
    static constexpr const char *setKBDBacklightLevel  = "MLCS";

    static constexpr const char *getKBDLang            = "GSKL";
    static constexpr const char *setKBDLang            = "SSKL";

    static constexpr const char *getAudioMutestatus    = "HAUM"; // EC
    static constexpr const char *setAudioMutestatus    = "SAUM";
    static constexpr const char *getAudioMuteLED       = "GSMS";
    static constexpr const char *setAudioMuteLED       = "SSMS"; // on_value = 1, off_value = 0
    static constexpr const char *setAudioMuteSupport   = "SHDA";
    static constexpr const char *getMicMuteLED         = "MMTG"; // HDMC: 0x00010000
    static constexpr const char *setMicMuteLED         = "MMTS"; // on_value = 2, off_value = 0, ? = 3

    static constexpr const char *getThermalControl     = "MHGT";
    static constexpr const char *setThermalControl     = "MHAT";

    static constexpr const char *getMultiModeState     = "GMMS"; // both from H_EC.CMMD
    static constexpr const char *getThermalModeState   = "GTMS";

    //    DSSG
    //    DSSS
    //    SBSG
    //    SBSS
    //    PBLG
    //    PBLS
    //    PMSG
    //    PMSS
    //    ISSG
    //    ISSS
    //    INSG
    //    INSS
    //
    //    Call FNSC:
    //    GMKS
    //    SMKS
    //    GSKL
    //    SSKL
    //    GHSL
    //    SHSL

    bool hotkey_legacy {false};
    /**
     * All events supported by DHKN
     */
    UInt32 hotkey_all_mask {0};

    /**
     * All adaptive events supported by DHKE
     */
    UInt32 hotkey_adaptive_all_mask {0};
    
    /**
     * Alternative (?) events supported by DHKF
     */
    UInt32 hotkey_alter_all_mask {0};

    UInt32 mutestate {0};
    UInt32 muteLEDstate {0};
    UInt32 muteLEDstateSaved {0};
    UInt32 micMuteLEDcap {0};
    UInt32 micMuteLEDstate {0};
    UInt32 micMuteLEDstateSaved {0};

    UInt32 thermalstate {0};

    bool LEDsupport {false};

    UInt32 batnum {BAT_ANY};

    bool ConservationMode {false};

    OSDictionary *KBDProperty {nullptr};
    /**
     *  Set single notification mask
     *
     *  @param i mask index
     *  @param all_mask available masks
     *  @param offset mask offset
     *  @param enable desired status
     *
     *  @return true if success
     */
    bool setNotificationMask(UInt32 i, UInt32 all_mask, UInt32 offset, bool enable=true);

    /**
     *  Get notification mask
     *
     *  @param index index of notification group
     *  @param KBDPresent dictionary for current settings
     */
    void getNotificationMask(UInt32 index, OSDictionary *KBDPresent);

    bool initVPC() APPLE_KEXT_OVERRIDE;
    void setPropertiesGated(OSObject* props) APPLE_KEXT_OVERRIDE;
    void updateAll() APPLE_KEXT_OVERRIDE;

    UInt32 validModes {0};

    /**
     *  Get yoga/multi mode
     *
     *  @return mode
     */
    UInt32 getYogaMode();
    
    /**
     *  Get thermal mode
     *
     *  @return mode
     */
    UInt32 getThermalMode();
    
    void updateVPC() APPLE_KEXT_OVERRIDE;
    bool exitVPC() APPLE_KEXT_OVERRIDE;
    
#ifndef ALTER
    /**
     *  Initialize SMC
     *
     *  @return true if success
     */
    inline void initSMC() APPLE_KEXT_OVERRIDE {smc = ThinkSMC::withDevice(this, ec);};
#endif

    bool updateBacklight(bool update=false) APPLE_KEXT_OVERRIDE;
    bool setBacklight(UInt32 level) APPLE_KEXT_OVERRIDE;

    /**
     *  Update battery conservation related setting
     *
     *  @param method method name to be executed
     *  @param bat dictionary for status
     *  @param update only update internal status when false
     *
     *  @return true if success
     */
    bool updateConservation(const char* method, OSDictionary *bat, bool update=true);
    
    /**
     *  Set battery conservation related setting
     *
     *  @param method method name to be executed
     *  @param value method argument (excluding battery number)
     *
     *  @return true if success
     */
    bool setConservation(const char* method, UInt8 value);

    bool updateAdaptiveKBD(int arg);

    /**
     *  Get FnLock status
     *
     *  @return Fnkey Status, false if false
     */
    bool updateFnLockStatus();

    /**
     *  Set hotkey reporting status
     *
     *  @param enable enable hotkey report
     *
     *  @return true if success
     */
    bool setHotkeyStatus(bool enable);

    /**
     *  Update HW Mute status
     *
     *  @param update only update internal status when false
     *
     *  @return true if success
     */
    bool updateMutestatus(bool update=true);

    /**
     *  Set HW Mute status
     *
     *  @param value status
     *
     *  @return true if success
     */
    bool setMutestatus(UInt32 value);

    /**
     *  Set Mute support
     *
     *  @param support off / on
     *
     *  @return true if success
     */
    bool setMuteSupport(bool support);

    /**
     *  Update Mute LED status
     *
     *  @param update only update internal status when false
     *
     *  @return true if success
     */
    bool updateMuteLEDStatus(bool update=true);

    /**
     *  Set Beep status
     *
     *  @param status length
     *
     *  @return true if success
     */
    bool setBeepStatus(UInt8 status);

    /**
     *  Enable Beep status
     *
     *  @param enable status
     *
     *  @return true if success
     */
    bool enableBeep(bool enable);

    /**
     *  Set LED status
     *
     *  @param status length
     *
     *  @return true if success
     */
    bool setLEDStatus(UInt8 status);

    /**
     *  Set Mute LED status
     *
     *  @param status on / off
     *
     *  @return true if success
     */
    bool setMuteLEDStatus(bool status);

    /**
     *  Update Mic Mute LED status
     *
     *  @param update only update internal status when false
     *
     *  @return true if success
     */
    bool updateMicMuteLEDStatus(bool update=true);

    /**
     *  Set Mic Mute LED status
     *
     *  @param status 3:blink, 2:on, 0:off
     *
     *  @return true if success
     */
    bool setMicMuteLEDStatus(UInt32 status);

    /**
     *  Update battery status
     *
     *  @param update only update internal status when false
     */
    void updateBattery(bool update=true);

    /**
     *  Set Fan control status
     *
     *  @param level 0 for automatic, 1-7 for level
     *
     *  @return true if success
     */
    bool setFanControl(int level);

    /**
     *  Update Fan control status
     *
     *  @param status TBD
     *  @param update only update internal status when false
     *
     *  @return true if success
     */
    bool updateFanControl(UInt32 status=0, bool update=true);

    /**
     *  Set _SI._SST status
     *
     *  @param value 0-4
     *
     *  @return true if success
     */
    bool setSSTStatus(UInt32 value);

    /**
     *  Update keyboard locale
     *
     *  @param update only update internal status when false
     *
     *  @return true if success
     */
    bool updateKBDLocale(bool update=true);

    /**
     *  Set keyboard locale
     *
     *  @param value locale
     *
     *  @return true if success
     */
    bool setKBDLocale(UInt32 value);

public:
    IOReturn message(UInt32 type, IOService *provider, void *argument) APPLE_KEXT_OVERRIDE;
    IOReturn setPowerState(unsigned long powerStateOrdinal, IOService * whatDevice) APPLE_KEXT_OVERRIDE;
};

#endif /* ThinkVPC_hpp */
