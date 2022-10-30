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

#define BAT_INFO_WMI_STRING             "c3a03776-51ac-49aa-ad0f-f2f7d62c3f3c"
#define GSENSOR_DATA_WMI_METHOD         "09b0ee6e-c3fd-4243-8da1-7911ff80bb8c"
#define GSENSOR_WMI_EVENT               "06129d99-6083-4164-81ad-f092f9d773a6"
#define GSENSOR_DATA_WMI_METHOD_EXT     "abbc0f6f-8ea1-11d1-00a0-c90629100000"
#define GSENSOR_WMI_EVENT_EXT           "abbc0f72-8ea1-11d1-00a0-c90629100000"
#define PAPER_LOOKING_WMI_EVENT         "56322276-8493-4ce8-a783-98c991274f5e"
#define GAME_ZONE_DATA_WMI_METHOD       "887b54e3-dddc-4b2c-8b88-68a26a8835d0"
#define GAME_ZONE_TEMP_EVENT            "bfd42481-aee3-4501-a107-afb68425c5f8"
#define GAME_ZONE_OC_EVENT              "d062906b-12d4-4510-999d-4831ee80e985"
#define GAME_ZONE_GPU_TEMP_EVENT        "bfd42481-aee3-4502-a107-afb68425c5f8"
#define GAME_ZONE_FAN_COOLING_EVENT     "bc72a435-e8c1-4275-b3e2-d8b8074aba59"
#define GAME_ZONE_KEYLOCK_STATUS_EVENT  "10afc6d9-ea8b-4590-a2e7-1cd3c84bb4b1"
#define GAME_ZONE_FAN_MODE_EVENT        "d320289e-8fea-41e0-86f9-611d83151b5f"
#define GAME_ZONE_POWER_CHARGE_EVENT    "d320289e-8fea-41e0-86f9-711d83151b5f"
#define GAME_ZONE_LIGHR_PROFILE_EVENT   "d320289e-8fea-41e0-86f9-811d83151b5f"
#define GAME_ZONE_THERMAL_MODE_EVENT    "d320289e-8fea-41e0-86f9-911d83151b5f"
#define GAME_ZONE_FAN_SETTING_EVENT     "d320289e-8fea-41e1-86f9-611d83151b5f"
#define SUPER_RES_DATA_WMI_METHOD       "77e614ed-f19e-46d6-a613-a8669fee1ff0"
#define SUPER_RES_WMI_EVENT             "95d1df76-d6c0-4e16-9193-7b2a849f3df2"

enum {
    GAME_ZONE_WMI_GET_TMP_IR        = 0x01,
    GAME_ZONE_WMI_GET_THERMAL_TABLE = 0x02,
    GAME_ZONE_WMI_SET_THERMAL_TABLE = 0x03,
    GAME_ZONE_WMI_CAP_GPU_OC        = 0x04,
    GAME_ZONE_WMI_GET_GPU_GPS       = 0x05,
    GAME_ZONE_WMI_SET_GPU_GPS       = 0x06,
    GAME_ZONE_WMI_GET_FAN_NUM       = 0x07,
    GAME_ZONE_WMI_GET_FAN_ONE       = 0x08,
    GAME_ZONE_WMI_GET_FAN_TWO       = 0x09,
    GAME_ZONE_WMI_GET_FAN_MAX       = 0x0A,
    GAME_ZONE_WMI_GET_VERSION       = 0x0B,
    GAME_ZONE_WMI_CAP_FAN_COOLING   = 0x0C,
    GAME_ZONE_WMI_SET_FAN_COOLING   = 0x0D,
    GAME_ZONE_WMI_CAP_CPU_OC        = 0x0E,
    GAME_ZONE_WMI_CAP_BIOS_OC       = 0x0F,
    GAME_ZONE_WMI_SET_BIOS_OC       = 0x10,
    GAME_ZONE_WMI_GET_TMP_TRIGGER   = 0x11,
    GAME_ZONE_WMI_GET_TMP_CPU       = 0x12,
    GAME_ZONE_WMI_GET_TMP_GPU       = 0x13,
    GAME_ZONE_WMI_GET_FAN_STA       = 0x14,
    GAME_ZONE_WMI_CAP_WIN_KEY       = 0x15,
    GAME_ZONE_WMI_SET_WIN_KEY       = 0x16,
    GAME_ZONE_WMI_GET_WIN_KEY       = 0x17,
    GAME_ZONE_WMI_CAP_TOUCHPAD      = 0x18,
    GAME_ZONE_WMI_SET_TOUCHPAD      = 0x19,
    GAME_ZONE_WMI_GET_TOUCHPAD      = 0x1A,
    GAME_ZONE_WMI_GET_GPU_NORM_MAX  = 0x1B,
    GAME_ZONE_WMI_GET_GPU_OC_MAX    = 0x1C,
    GAME_ZONE_WMI_GET_GPU_OC_TYPE   = 0x1D,
    GAME_ZONE_WMI_CAP_KEYBOARD      = 0x1E,
    GAME_ZONE_WMI_CAP_MEM_OC_INFO   = 0x1F,
    GAME_ZONE_WMI_CAP_WATER_COOLING = 0x20,
    GAME_ZONE_WMI_SET_WATER_COOLING = 0x21,
    GAME_ZONE_WMI_GET_WATER_COOLING = 0x22,
    GAME_ZONE_WMI_CAP_LIGHTING      = 0x23,
    GAME_ZONE_WMI_SET_LIGHTING      = 0x24,
    GAME_ZONE_WMI_GET_LIGHTING      = 0x25,
    GAME_ZONE_WMI_GET_MARCOKEY_CODE = 0x26,
    GAME_ZONE_WMI_GET_MARCOKEY_CNT  = 0x27,
    GAME_ZONE_WMI_CAP_GSYNC         = 0x28,
    GAME_ZONE_WMI_GET_GSYNC         = 0x29,
    GAME_ZONE_WMI_SET_GSYNC         = 0x2A,
    GAME_ZONE_WMI_CAP_SMARTFAN      = 0x2B,
    GAME_ZONE_WMI_SET_SMARTFAN_MODE = 0x2C,
    GAME_ZONE_WMI_GET_SMARTFAN_MODE = 0x2D,
    GAME_ZONE_WMI_GET_SMARTFAN_STA  = 0x2F,
    GAME_ZONE_WMI_GET_CHARGE_MODE   = 0x30,
    GAME_ZONE_WMI_CAP_OVER_DRIVE    = 0x31,
    GAME_ZONE_WMI_GET_OVER_DRIVE    = 0x32,
    GAME_ZONE_WMI_SET_OVER_DRIVE    = 0x33,
    GAME_ZONE_WMI_SET_LIGHT_CTL     = 0x34,
    GAME_ZONE_WMI_SET_DDS_CTL       = 0x35,
    GAME_ZONE_WMI_RET_OC_VAL        = 0x36,
    GAME_ZONE_WMI_GET_THERMAL_MODE  = 0x37
};

enum
{
    GSENSOR_DATA_WMI_UASGE_MODE = 1,
    GSENSOR_DATA_WMI_AXIS_X     = 2,
    GSENSOR_DATA_WMI_AXIS_Y     = 3,
    GSENSOR_DATA_WMI_AXIS_Z     = 4,
    GSENSOR_DATA_WMI_ANGLE_4    = 5,
    GSENSOR_DATA_WMI_ANGLE_5    = 6,
    GSENSOR_DATA_WMI_ANGLE_6    = 7
};

enum
{
    kYogaMode_laptop = 1,   // 0-90 degree
    kYogaMode_tablet = 2,   // 0/360 degree
    kYogaMode_stand  = 3,   // 180-360 degree, ∠ , screen face up
    kYogaMode_tent   = 4    // 180-360 degree, ∧ , screen upside down, trigger rotation?
} kYogaMode;

enum
{
    SUPER_RES_DATA_WMI_VALUE     = 1,
    SUPER_RES_DATA_WMI_START     = 2,
    SUPER_RES_DATA_WMI_STOP      = 3,
    SUPER_RES_DATA_WMI_CAP       = 4,
    SUPER_RES_DATA_WMI_RESERVED1 = 5,
    SUPER_RES_DATA_WMI_RESERVED2 = 6
};

class IdeaWMIYoga : public YogaWMI
{
    typedef YogaWMI super;
    OSDeclareDefaultStructors(IdeaWMIYoga)

    static constexpr const char *feature = "Yoga Mode Control";
    UInt32 YogaEvent {0xd0};
    bool extension {false};
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

class IdeaWMISuperRes : public YogaWMI
{
    typedef YogaWMI super;
    OSDeclareDefaultStructors(IdeaWMISuperRes)

    static constexpr const char *feature = "Super Resolution";
    UInt32 SREvent {0xd0};
    virtual void setPropertiesGated(OSObject* props);

    void processWMI() APPLE_KEXT_OVERRIDE;
    void ACPIEvent(UInt32 argument) APPLE_KEXT_OVERRIDE;
    const char* registerEvent(OSString *guid, UInt32 id) APPLE_KEXT_OVERRIDE;

public:
    virtual IOReturn setProperties(OSObject* props) APPLE_KEXT_OVERRIDE;
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
    UInt32 tempEvent {0xd0};
    UInt32 OCEvent   {0xd1};
    UInt32 GPUEvent  {0xe0};
    UInt32 fanEvent  {0xe1};
    UInt32 keyEvent  {0xe2};
    UInt32 smartFanModeEvent  {0xe3};

    UInt32 smartFanMode {0};

    bool getGamzeZoneData(UInt32 query, UInt32 *result);

#ifndef ALTER
    friend class IdeaSMC;
#endif

    void processWMI() APPLE_KEXT_OVERRIDE;
    void ACPIEvent(UInt32 argument) APPLE_KEXT_OVERRIDE;
    const char* registerEvent(OSString *guid, UInt32 id) APPLE_KEXT_OVERRIDE;
};

#endif /* IdeaWMI_hpp */
