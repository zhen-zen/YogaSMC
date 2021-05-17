//  SPDX-License-Identifier: GPL-2.0-only
//
//  ThinkEvents.h
//  YogaSMC
//
//  Created by Zhen on 10/15/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef ThinkEvents_h
#define ThinkEvents_h

#include <libkern/OSTypes.h>

// mostly from linux/drivers/platform/x86/thinkpad_acpi.c

/* HKEY events */
enum tpacpi_hkey_event_t : UInt32 {
    /* Hotkey-related */
    TP_HKEY_EV_HOTKEY_BASE      = 0x1001, /* First hotkey (FN+F1, _Q10) */
    TP_HKEY_EV_SLEEP            = 0x1004, /* Sleep button (F4, _Q13) */
    TP_HKEY_EV_NETWORK          = 0x1005, /* Network (F8, _Q64) */
    TP_HKEY_EV_DISPLAY          = 0x1007, /* Dual Display (F7, _Q16/_Q19) */
    TP_HKEY_EV_BRGHT_UP         = 0x1010, /* Brightness up (F6, _Q14/_Q1C) */
    TP_HKEY_EV_BRGHT_DOWN       = 0x1011, /* Brightness down (F5, _Q15/_Q1D) */
    TP_HKEY_EV_KBD_LIGHT        = 0x1012, /* Thinklight/kbd backlight (Fn+Space, _Q1F) */
    TP_HKEY_EV_VOL_UP           = 0x1015, /* Volume up or unmute */
    TP_HKEY_EV_VOL_DOWN         = 0x1016, /* Volume down or unmute */
    TP_HKEY_EV_VOL_MUTE         = 0x1017, /* Mixer output mute */
    TP_HKEY_EV_MIC_MUTE         = 0x101B, /* Microphone Mute (F4, _Q6A) */
    TP_HKEY_EV_SETTING          = 0x101D, /* Settings (F9, _Q66) */
    TP_HKEY_EV_SEARCH           = 0x101E, /* Search (F10, _Q67) */
    TP_HKEY_EV_MISSION          = 0x101F, /* All open apps/Mission Control (F11, _Q68) */
    TP_HKEY_EV_APPS             = 0x1020, /* All programs/Launchpad (F12, _Q69) */

    /* Hotkey-related (preset masks) */
    TP_HKEY_EV_STAR             = 0x1311, /* Star (F12, _Q62) */
    TP_HKEY_EV_BLUETOOTH        = 0x1314, /* Bluetooth (F10, _Q60) */
    TP_HKEY_EV_KEYBOARD         = 0x1315, /* Keyboard (F11, _Q61) */

    /* Reasons for waking up from S3/S4 */
    TP_HKEY_EV_WKUP_S3_UNDOCK    = 0x2304, /* undock requested, S3 */
    TP_HKEY_EV_WKUP_S4_UNDOCK    = 0x2404, /* undock requested, S4 */
    TP_HKEY_EV_WKUP_S3_BAYEJ     = 0x2305, /* bay ejection req, S3 */
    TP_HKEY_EV_WKUP_S4_BAYEJ     = 0x2405, /* bay ejection req, S4 */
    TP_HKEY_EV_WKUP_S3_BATLOW    = 0x2313, /* battery empty, S3 */
    TP_HKEY_EV_WKUP_S4_BATLOW    = 0x2413, /* battery empty, S4 */

    /* Auto-sleep after eject request */
    TP_HKEY_EV_BAYEJ_ACK        = 0x3003, /* bay ejection complete */
    TP_HKEY_EV_UNDOCK_ACK       = 0x4003, /* undock complete */

    /* Misc bay events */
    TP_HKEY_EV_OPTDRV_EJ        = 0x3006, /* opt. drive tray ejected */
    TP_HKEY_EV_HOTPLUG_DOCK     = 0x4010, /* docked into hotplug dock or port replicator */
    TP_HKEY_EV_HOTPLUG_UNDOCK   = 0x4011, /* undocked from hotplug dock or port replicator */

    /* User-interface events */
    TP_HKEY_EV_LID_CLOSE          = 0x5001, /* laptop lid closed */
    TP_HKEY_EV_LID_OPEN           = 0x5002, /* laptop lid opened */
    TP_HKEY_EV_TABLET_TABLET      = 0x5009, /* tablet swivel up */
    TP_HKEY_EV_TABLET_NOTEBOOK    = 0x500A, /* tablet swivel down */
    TP_HKEY_EV_TABLET_CHANGED     = 0x60C0, /* X1 Yoga (2016):
                           * enter/leave tablet mode
                           */
    TP_HKEY_EV_PEN_INSERTED       = 0x500B, /* tablet pen inserted */
    TP_HKEY_EV_PEN_REMOVED        = 0x500C, /* tablet pen removed */
    TP_HKEY_EV_BRGHT_CHANGED      = 0x5010, /* backlight control event */

    /* Key-related user-interface events */
    TP_HKEY_EV_KEY_NUMLOCK        = 0x6000, /* NumLock key pressed */
    TP_HKEY_EV_KEY_FN             = 0x6005, /* Fn key pressed? E420 */
    TP_HKEY_EV_KEY_FN_ESC         = 0x6060, /* Fn+Esc key pressed X240 */


    /* Thermal events */
    TP_HKEY_EV_ALARM_BAT_HOT        = 0x6011, /* battery too hot */
    TP_HKEY_EV_ALARM_BAT_XHOT       = 0x6012, /* battery critically hot */
    TP_HKEY_EV_ALARM_SENSOR_HOT     = 0x6021, /* sensor too hot */
    TP_HKEY_EV_ALARM_SENSOR_XHOT    = 0x6022, /* sensor critically hot */
    TP_HKEY_EV_THM_TABLE_CHANGED    = 0x6030, /* windows; thermal table changed */
    TP_HKEY_EV_THM_CSM_COMPLETED    = 0x6032, /* windows; thermal control set
                           * command completed. Related to
                           * AML DYTC */
    TP_HKEY_EV_BACKLIGHT_CHANGED     = 0x6050, /* backlight changed */
    TP_HKEY_EV_LID_STATUS_CHANGED    = 0x60D0, /* lid status changed */
    TP_HKEY_EV_THM_TRANSFM_CHANGED   = 0x60F0, /* windows; thermal transformation
                           * changed. Related to AML GMTS */

    /* AC-related events */
    TP_HKEY_EV_AC_CHANGED        = 0x6040, /* AC status changed */

    /* Further user-interface events */
    TP_HKEY_EV_PALM_DETECTED      = 0x60B0, /* palm hoveres keyboard */
    TP_HKEY_EV_PALM_UNDETECTED    = 0x60B1, /* palm removed */

    /* Misc */
    TP_HKEY_EV_RFKILL_CHANGED    = 0x7000, /* rfkill switch changed */
};

#endif /* ThinkEvents_h */
