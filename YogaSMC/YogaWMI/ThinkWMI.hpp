//  SPDX-License-Identifier: GPL-2.0-only
//
//  ThinkWMI.hpp
//  YogaSMC
//
//  Created by Zhen on 6/27/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef ThinkWMI_hpp
#define ThinkWMI_hpp

#include "../YogaSMC.hpp"
#include "../YogaVPC.hpp"

// from linux/drivers/platform/x86/ideapad-laptop.c
// TODO: https://github.com/lenovo/thinklmi

enum {
    BAT_ANY = 0,
    BAT_PRIMARY = 1,
    BAT_SECONDARY = 2
};

class ThinkWMI : public YogaWMI
{
    typedef YogaWMI super;
    OSDeclareDefaultStructors(ThinkWMI)

private:
    bool TouchPadenabled = 0;

    /**
     *  Related ACPI methods
     */
    static constexpr const char *getHKEYversion        = "MHKV";

    OSString *pnp = OSString::withCString("LEN0268");

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

    bool initVPC() override;
    inline OSString *getPnp() override {return OSString::withCString(PnpDeviceIdVPCThink);};

    int batnum = BAT_ANY;
    bool ConservationMode;

    bool updateConservation(const char* method, bool update=true);
    
    bool toggleConservation();
    bool updateAdaptiveKBD(int arg);
    bool updateMutestatus();

public:
    IOService *probe(IOService *provider, SInt32 *score) override;
    IOReturn setProperties(OSObject* props) override;
};

#endif /* ThinkWMI_hpp */
