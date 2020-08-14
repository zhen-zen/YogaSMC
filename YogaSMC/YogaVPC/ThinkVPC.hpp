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

// from linux/drivers/platform/x86/ideapad-laptop.c and ibm-acpi

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

    static constexpr const char *getAdaptiveKBD        = "MHKA";
    static constexpr const char *getAudioMutestatus    = "HAUM";
    static constexpr const char *setAudioMutestatus    = "SAUM";
    static constexpr const char *getAudioMuteLED       = "GSMS";
    static constexpr const char *setAudioMuteLED       = "SSMS"; // on_value = 1, off_value = 0
    static constexpr const char *setAudioMuteSupport   = "SHDA";
    static constexpr const char *getMicMuteLED         = "MMTG"; // HDMC: 0x00010000
    static constexpr const char *setMicMuteLED         = "MMTS"; // on_value = 2, off_value = 0, ? = 3

    static constexpr const char *setControl            = "MHAT";

    UInt32 mutestate;
    UInt32 muteLEDstate;
    UInt32 micMuteLEDstate;

    UInt32 mutestate_orig;

    bool initVPC() APPLE_KEXT_OVERRIDE;
    void setPropertiesGated(OSObject* props) APPLE_KEXT_OVERRIDE;
    void updateAll() APPLE_KEXT_OVERRIDE;
//    void updateVPC() APPLE_KEXT_OVERRIDE;
    bool exitVPC() APPLE_KEXT_OVERRIDE;

    int batnum = BAT_ANY;
    bool ConservationMode;

    bool updateConservation(const char* method, bool update=true);
    
    bool setConservation(const char* method, UInt32 value);

    bool updateAdaptiveKBD(int arg);

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
     *  @param battery battery number     BAT_ANY = 0, BAT_PRIMARY = 1, BAT_SECONDARY = 2
     */
    void updateBattery(int battery);

    /**
     *  Set Fan Control
     *
     *  @param level 0 for automatic, 1-7 for level
     *
     *  @return true if success
     */
    bool setFanControl(int level);

    friend class ThinkWMI;

public:
    static ThinkVPC* withDevice(IOACPIPlatformDevice *device, OSString *pnp);
};

#endif /* ThinkVPC_hpp */
