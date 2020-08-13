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

// from linux/drivers/platform/x86/ideapad-laptop.c

enum tpacpi_mute_btn_mode {
    TP_EC_MUTE_BTN_LATCH  = 0,    /* Mute mutes; up/down unmutes */
    /* We don't know what mode 1 is. */
    TP_EC_MUTE_BTN_NONE   = 2,    /* Mute and up/down are just keys */
    TP_EC_MUTE_BTN_TOGGLE = 3,    /* Mute toggles; up/down unmutes */
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

    static constexpr const char *setControl            = "MHAT";

    UInt32 mutestate;

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
     *  Update Mute status
     *
     *  @param update only update internal status when false
     *
     *  @return true if success
     */
    bool updateMutestatus(bool update=true);
    bool setMutestatus(UInt32 value);

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
