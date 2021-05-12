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

enum {
    GAME_ZONE_WMI_GET_TMP_IR   = 0x01,
    GAME_ZONE_WMI_GET_TTBL_ID  = 0x02,
    GAME_ZONE_WMI_SET_TTBL_ID  = 0x03,
    GAME_ZONE_WMI_CAP_GPU_OC   = 0x04,
    GAME_ZONE_WMI_GET_GPU_GPS  = 0x05,
    GAME_ZONE_WMI_SET_GPU_GPS  = 0x06,
    GAME_ZONE_WMI_GET_FAN_NUM  = 0x07,
    GAME_ZONE_WMI_GET_FAN_ONE  = 0x08,
    GAME_ZONE_WMI_GET_FAN_TWO  = 0x09,
    GAME_ZONE_WMI_GET_FAN_MAX  = 0x0A,
    GAME_ZONE_WMI_GET_VERSION  = 0x0B,
    GAME_ZONE_WMI_CAP_FAN_SET  = 0x0C,
    GAME_ZONE_WMI_SET_FAN_STA  = 0x0D,
    GAME_ZONE_WMI_CAP_CPU_OC   = 0x0E,
    GAME_ZONE_WMI_CAP_BIOS_OC  = 0x0F,
    GAME_ZONE_WMI_SET_BIOS_OC  = 0x10,
    GAME_ZONE_WMI_GET_TRIGGER  = 0x11,
    GAME_ZONE_WMI_GET_TMP_CPU  = 0x12,
    GAME_ZONE_WMI_GET_TMP_GPU  = 0x13,
    GAME_ZONE_WMI_GET_FAN_STA  = 0x14,
    GAME_ZONE_WMI_CAP_WIN_KEY  = 0x15,
    GAME_ZONE_WMI_SET_WIN_KEY  = 0x16,
    GAME_ZONE_WMI_GET_WIN_KEY  = 0x17,
    GAME_ZONE_WMI_CAP_TOUCHPAD = 0x18,
    GAME_ZONE_WMI_SET_TOUCHPAD = 0x19,
    GAME_ZONE_WMI_GET_TOUCHPAD = 0x1A,
    GAME_ZONE_WMI_GET_GPU_MAX  = 0x1B,
    GAME_ZONE_WMI_GET_GPU_OCM  = 0x1C,
    GAME_ZONE_WMI_GET_GPU_TYPE = 0x1D,
    GAME_ZONE_WMI_CAP_KEYBOARD = 0x1E
};

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

    static constexpr const char *feature = "Game Zone";

    bool getGamzeZoneData(UInt32 query, UInt32 *result);

    void processWMI() APPLE_KEXT_OVERRIDE;
    void ACPIEvent(UInt32 argument) APPLE_KEXT_OVERRIDE;
    const char* registerEvent(OSString *guid, UInt32 id) APPLE_KEXT_OVERRIDE;
};

#endif /* IdeaWMI_hpp */
