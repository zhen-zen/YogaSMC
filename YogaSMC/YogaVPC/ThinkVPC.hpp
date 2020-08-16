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

/* HKEY events */
enum tpacpi_hkey_event_t {
    /* Hotkey-related */
    TP_HKEY_EV_HOTKEY_BASE        = 0x1001, /* first hotkey (FN+F1) */
    TP_HKEY_EV_BRGHT_UP        = 0x1010, /* Brightness up */
    TP_HKEY_EV_BRGHT_DOWN        = 0x1011, /* Brightness down */
    TP_HKEY_EV_KBD_LIGHT        = 0x1012, /* Thinklight/kbd backlight */
    TP_HKEY_EV_VOL_UP        = 0x1015, /* Volume up or unmute */
    TP_HKEY_EV_VOL_DOWN        = 0x1016, /* Volume down or unmute */
    TP_HKEY_EV_VOL_MUTE        = 0x1017, /* Mixer output mute */

    /* Reasons for waking up from S3/S4 */
    TP_HKEY_EV_WKUP_S3_UNDOCK    = 0x2304, /* undock requested, S3 */
    TP_HKEY_EV_WKUP_S4_UNDOCK    = 0x2404, /* undock requested, S4 */
    TP_HKEY_EV_WKUP_S3_BAYEJ    = 0x2305, /* bay ejection req, S3 */
    TP_HKEY_EV_WKUP_S4_BAYEJ    = 0x2405, /* bay ejection req, S4 */
    TP_HKEY_EV_WKUP_S3_BATLOW    = 0x2313, /* battery empty, S3 */
    TP_HKEY_EV_WKUP_S4_BATLOW    = 0x2413, /* battery empty, S4 */

    /* Auto-sleep after eject request */
    TP_HKEY_EV_BAYEJ_ACK        = 0x3003, /* bay ejection complete */
    TP_HKEY_EV_UNDOCK_ACK        = 0x4003, /* undock complete */

    /* Misc bay events */
    TP_HKEY_EV_OPTDRV_EJ        = 0x3006, /* opt. drive tray ejected */
    TP_HKEY_EV_HOTPLUG_DOCK        = 0x4010, /* docked into hotplug dock
                             or port replicator */
    TP_HKEY_EV_HOTPLUG_UNDOCK    = 0x4011, /* undocked from hotplug
                             dock or port replicator */

    /* User-interface events */
    TP_HKEY_EV_LID_CLOSE        = 0x5001, /* laptop lid closed */
    TP_HKEY_EV_LID_OPEN        = 0x5002, /* laptop lid opened */
    TP_HKEY_EV_TABLET_TABLET    = 0x5009, /* tablet swivel up */
    TP_HKEY_EV_TABLET_NOTEBOOK    = 0x500a, /* tablet swivel down */
    TP_HKEY_EV_TABLET_CHANGED    = 0x60c0, /* X1 Yoga (2016):
                           * enter/leave tablet mode
                           */
    TP_HKEY_EV_PEN_INSERTED        = 0x500b, /* tablet pen inserted */
    TP_HKEY_EV_PEN_REMOVED        = 0x500c, /* tablet pen removed */
    TP_HKEY_EV_BRGHT_CHANGED    = 0x5010, /* backlight control event */

    /* Key-related user-interface events */
    TP_HKEY_EV_KEY_NUMLOCK        = 0x6000, /* NumLock key pressed */
    TP_HKEY_EV_KEY_FN        = 0x6005, /* Fn key pressed? E420 */
    TP_HKEY_EV_KEY_FN_ESC           = 0x6060, /* Fn+Esc key pressed X240 */

    /* Thermal events */
    TP_HKEY_EV_ALARM_BAT_HOT    = 0x6011, /* battery too hot */
    TP_HKEY_EV_ALARM_BAT_XHOT    = 0x6012, /* battery critically hot */
    TP_HKEY_EV_ALARM_SENSOR_HOT    = 0x6021, /* sensor too hot */
    TP_HKEY_EV_ALARM_SENSOR_XHOT    = 0x6022, /* sensor critically hot */
    TP_HKEY_EV_THM_TABLE_CHANGED    = 0x6030, /* windows; thermal table changed */
    TP_HKEY_EV_THM_CSM_COMPLETED    = 0x6032, /* windows; thermal control set
                           * command completed. Related to
                           * AML DYTC */
    TP_HKEY_EV_THM_TRANSFM_CHANGED  = 0x60F0, /* windows; thermal transformation
                           * changed. Related to AML GMTS */

    /* AC-related events */
    TP_HKEY_EV_AC_CHANGED        = 0x6040, /* AC status changed */

    /* Further user-interface events */
    TP_HKEY_EV_PALM_DETECTED    = 0x60b0, /* palm hoveres keyboard */
    TP_HKEY_EV_PALM_UNDETECTED    = 0x60b1, /* palm removed */

    /* Misc */
    TP_HKEY_EV_RFKILL_CHANGED    = 0x7000, /* rfkill switch changed */
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
    static constexpr const char *setHKEYstatus         = "MHKC";
    static constexpr const char *getHKEYmask           = "MHKN";
    static constexpr const char *setHKEYmask           = "MHKM";

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

    static constexpr const char *getAudioMutestatus    = "HAUM";
    static constexpr const char *setAudioMutestatus    = "SAUM";
    static constexpr const char *getAudioMuteLED       = "GSMS";
    static constexpr const char *setAudioMuteLED       = "SSMS"; // on_value = 1, off_value = 0
    static constexpr const char *setAudioMuteSupport   = "SHDA";
    static constexpr const char *getMicMuteLED         = "MMTG"; // HDMC: 0x00010000
    static constexpr const char *setMicMuteLED         = "MMTS"; // on_value = 2, off_value = 0, ? = 3

    static constexpr const char *setControl            = "MHAT";

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
    UInt32 micMuteLEDstate {0};

    UInt32 mutestate_orig {0};

    UInt32 batnum {BAT_ANY};

    bool ConservationMode {false};

    /**
     *  Set single notification mask
     *
     *  @param i mask index
     *  @param all_mask available masks
     *  @param offset mask offset
     *
     *  @return true if success
     */
    IOReturn setNotificationMask(UInt32 i, UInt32 all_mask, UInt32 offset);
    
    bool initVPC() APPLE_KEXT_OVERRIDE;
    void setPropertiesGated(OSObject* props) APPLE_KEXT_OVERRIDE;
    void updateAll() APPLE_KEXT_OVERRIDE;
    void updateVPC() APPLE_KEXT_OVERRIDE;
    bool exitVPC() APPLE_KEXT_OVERRIDE;

    bool updateBacklight(bool update=true) APPLE_KEXT_OVERRIDE;
    bool setBacklight(UInt32 level) APPLE_KEXT_OVERRIDE;

    bool updateConservation(const char* method, bool update=true);
    
    bool setConservation(const char* method, UInt32 value);

    bool updateAdaptiveKBD(int arg);

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
    IOReturn message(UInt32 type, IOService *provider, void *argument) APPLE_KEXT_OVERRIDE;
};

#endif /* ThinkVPC_hpp */
