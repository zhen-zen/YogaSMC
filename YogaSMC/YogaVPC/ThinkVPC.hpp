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

#include "../YogaVPC.hpp"

// from linux/drivers/platform/x86/ideapad-laptop.c
// TODO: https://github.com/lenovo/thinklmi

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
    static constexpr const char *getMutestatus         = "HAUM";

    UInt32 mutestate;

    bool initVPC() APPLE_KEXT_OVERRIDE;

//    void updateVPC() APPLE_KEXT_OVERRIDE;

    int batnum = BAT_ANY;
    bool ConservationMode;

    bool updateConservation(const char* method, bool update=true);
    
    bool toggleConservation();
    bool updateAdaptiveKBD(int arg);
    bool updateMutestatus();

    void updateBattery(int battery);
    
    void setPropertiesGated(OSObject* props) APPLE_KEXT_OVERRIDE;

    void updateAll();

    friend class ThinkWMI;

public:
    static ThinkVPC* withDevice(IOACPIPlatformDevice *device, OSString *pnp);
};

#endif /* ThinkVPC_hpp */
