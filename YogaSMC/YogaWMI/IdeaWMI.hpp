//  SPDX-License-Identifier: GPL-2.0-only
//
//  IdeaWMI.hpp
//  YogaSMC
//
//  Created by Zhen on 2020/6/23.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef IdeaWMI_hpp
#define IdeaWMI_hpp

#include "YogaWMI.hpp"

#define WBAT_BAT0_BatMaker 0
#define WBAT_BAT0_HwId     1
#define WBAT_BAT0_MfgDate  2
#define WBAT_BAT1_BatMaker 3
#define WBAT_BAT1_HwId     4
#define WBAT_BAT1_MfgDate  5

#define BAT_INFO_WMI_STRING         "c3a03776-51ac-49aa-ad0f-f2f7d62c3f3c"
#define GSENSOR_DATA_WMI_METHOD     "09b0ee6e-c3fd-4243-8da1-7911ff80bb8c"
#define GSENSOR_WMI_EVENT           "06129d99-6083-4164-81ad-f092f9d773a6"
#define PAPER_LOOKING_WMI_EVENT     "56322276-8493-4ce8-a783-98c991274f5e"
#define GAME_ZONE_DATA_WMI_METHOD   "887b54e3-dddc-4b2c-8b88-68a26a8835d0"

#define GAME_ZONE_DATA_WMI_GOC_CAP   0x04
#define GAME_ZONE_DATA_WMI_FAN_NUM   0x07
#define GAME_ZONE_DATA_WMI_FAN_MAX   0x0a
#define GAME_ZONE_DATA_WMI_VERSION   0x0b
#define GAME_ZONE_DATA_WMI_FAN_CAP   0x0c
#define GAME_ZONE_DATA_WMI_COC_CAP   0x0e

#define kIOACPIMessageYMC       0xd0
#define kIOACPIMessageGZTemp    0xd0

enum
{
    kYogaMode_laptop = 1,   // 0-90 degree
    kYogaMode_tablet = 2,   // 0/360 degree
    kYogaMode_stand  = 3,   // 180-360 degree, ∠ , screen face up
    kYogaMode_tent   = 4    // 180-360 degree, ∧ , screen upside down, trigger rotation?
} kYogaMode;

class IdeaWMIYoga : public YogaWMI
{
    typedef YogaWMI super;
    OSDeclareDefaultStructors(IdeaWMIYoga)

    static constexpr const char *feature = "Yoga Mode Control";
    UInt32 YogaEvent {0xd0};
    /**
     *  Current Yoga Mode, see kYogaMode
     */
    int YogaMode {kYogaMode_laptop};

    /**
     *  Update Yoga Mode
     */
    void updateYogaMode();

    void processWMI() APPLE_KEXT_OVERRIDE;
    void ACPIEvent(UInt32 argument) APPLE_KEXT_OVERRIDE;
    const char* registerEvent(OSString *guid, UInt32 id) APPLE_KEXT_OVERRIDE;
    inline virtual bool PMSupport() APPLE_KEXT_OVERRIDE {return true;};

public:
    void stop(IOService *provider) APPLE_KEXT_OVERRIDE;

    IOReturn setPowerState(unsigned long powerStateOrdinal, IOService * whatDevice) APPLE_KEXT_OVERRIDE;
};

class IdeaWMIPaper : public YogaWMI
{
    typedef YogaWMI super;
    OSDeclareDefaultStructors(IdeaWMIPaper)

    static constexpr const char *feature = "Paper Display";
    UInt32 paperEvent {0x80};

    void processWMI() APPLE_KEXT_OVERRIDE;
    void ACPIEvent(UInt32 argument) APPLE_KEXT_OVERRIDE;
    const char* registerEvent(OSString *guid, UInt32 id) APPLE_KEXT_OVERRIDE;
};

class IdeaWMIBattery : public YogaWMI
{
    typedef YogaWMI super;
    OSDeclareDefaultStructors(IdeaWMIBattery)

    /**
     *  Parse Battery Info
     *
     *  @param method method name to be executed
     *  @param bat array for status
     *
     *  @return true if success
     */
    bool getBatteryInfo (UInt32 index, OSArray *bat);

    void processWMI() APPLE_KEXT_OVERRIDE;
};

class IdeaWMIGameZone : public YogaWMI
{
    typedef YogaWMI super;
    OSDeclareDefaultStructors(IdeaWMIGameZone)

    bool getGamzeZoneData(UInt32 query, UInt32 *result);

    void processWMI() APPLE_KEXT_OVERRIDE;
    void ACPIEvent(UInt32 argument) APPLE_KEXT_OVERRIDE;
};

#endif /* IdeaWMI_hpp */
