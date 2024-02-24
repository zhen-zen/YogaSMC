//  SPDX-License-Identifier: GPL-2.0-only
//
//  message.h
//  YogaSMC
//
//  Created by Zhen on 7/26/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef message_h
#define message_h

#define kIOACPIMessageReserved 0x80

#define PnpDeviceIdEC       "PNP0C09"
#define PnpDeviceIdWMI      "PNP0C14"

#define kIOPMNumberPowerStates     2

static IOPMPowerState IOPMPowerStates[kIOPMNumberPowerStates] = {
    {1, kIOServicePowerCapabilityOff, kIOServicePowerCapabilityOff, kIOServicePowerCapabilityOff, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, kIOServicePowerCapabilityOn, kIOServicePowerCapabilityOn, kIOServicePowerCapabilityOn, 0, 0, 0, 0, 0, 0, 0, 0}
};

#define kDeliverNotifications   "RM,deliverNotifications"

#define alwaysOnUSBPrompt "AlwaysOnUSBMode"
#define autoBacklightPrompt "AutoBacklight"
#define batteryPrompt "Battery"
#define backlightPrompt "BacklightLevel"
#define backlightTimeoutPrompt "BacklightTimeout"
#define beepPrompt "Beep"
#define SSTPrompt "SST"
#define conservationPrompt "ConservationMode"
#define clamshellPrompt "ClamshellMode"
#define DYTCPrompt "DYTCMode"
#define DYTCFuncPrompt "DYTCFuncMode"
#define DYTCPerfPrompt "DYTCPerfMode"
#define DYTCPSCPrompt "DYTCPSCMode"
#define ECLockPrompt "ECLock"
#define FnKeyPrompt "FnlockMode"
#define fanControlPrompt "FanControl"
#define fanSpeedPrompt "FanSpeed"
#define keyboardPrompt "KeyboardMode"
#define HotKeyPrompt "HotKey"
#define LEDPrompt "LED"
#define localePrompt "KeyboardLocale"
#define mutePrompt "Mute"
#define muteLEDPrompt "MuteLED"
#define muteSupportPrompt "MuteSupport"
#define micMuteLEDPrompt "MicMuteLED"
#define VPCPrompt "VPCconfig"
#define rapidChargePrompt "RapidChargeMode"
#define readECPrompt "ReadEC"
#define resetPrompt "reset"
#define writeECPrompt "WriteEC"
#define updatePrompt "Update"
#define updateSensorPrompt "UpdateSensor"

#define initFailure "%s evaluation failed, exiting"
#define updateFailure "%s evaluation failed"
#define updateSuccess "%s 0x%x"
#define toggleError   "%s toggle error code: 0x%x"
#define toggleFailure "%s toggle failed"
#define toggleSuccess "%s set to 0x%x: %s"

#define notSupported "%s not supported"
#define valueMatched "%s already %x"
#define valueInvalid "Invalid value for %s"
#define valueUnknown "Unknown value for %s: %d"

#define timeoutPrompt "%s timeout 0x%x"

enum
{
    // from keyboard to mouse/touchpad
    kSMC_setDisableTouchpad = iokit_vendor_specific_msg(100),   // set disable/enable touchpad (data is bool*)
    kSMC_getDisableTouchpad = iokit_vendor_specific_msg(101),   // get disable/enable touchpad (data is bool*)

    // from sensor to keyboard
    kSMC_setKeyboardStatus  = iokit_vendor_specific_msg(200),   // set disable/enable keyboard (data is bool*)
    kSMC_getKeyboardStatus  = iokit_vendor_specific_msg(201),   // get disable/enable keyboard (data is bool*)
    kSMC_notifyKeystroke    = iokit_vendor_specific_msg(202),   // notify of key press (data is PS2KeyInfo*)

    // SMC message types
    kSMC_VPCType            = iokit_vendor_specific_msg(500),   // set loaded VPC type (data is UInt32*)
    kSMC_YogaEvent          = iokit_vendor_specific_msg(501),   // set Yoga mode (data is UInt32*)
    kSMC_FnlockEvent        = iokit_vendor_specific_msg(502),   // notify Fnlock event
    kSMC_getConservation    = iokit_vendor_specific_msg(503),   // get conservation mode (data is bool*)
    kSMC_setConservation    = iokit_vendor_specific_msg(504),   // set conservation mode (data is bool*)
    kSMC_smartFanEvent      = iokit_vendor_specific_msg(505),   // set Yoga mode (data is UInt32*)
};

// from VoodooPS2
enum
{
    // from keyboard to mouse/touchpad
    kPS2M_notifyKeyPressed = iokit_vendor_specific_msg(102),     // notify of time key pressed (data is PS2KeyInfo*)

    kPS2M_notifyKeyTime = iokit_vendor_specific_msg(110),        // notify of timestamp a non-modifier key was pressed (data is uint64_t*)

    kPS2M_resetTouchpad = iokit_vendor_specific_msg(151),        // Force touchpad reset (data is int*)
};

typedef struct PS2KeyInfo
{
    uint64_t time;
    UInt16  adbKeyCode;
    bool    goingDown;
    bool    eatKey;
} PS2KeyInfo;

enum hid_adb_codes {
    ADB_PLAY_PAUSE        =  0x34,
    ADB_LEFT_META         =  0x37,
    ADB_NUM_LOCK          =  0x47,
    ADB_VOLUME_UP         =  0x48,
    ADB_VOLUME_DOWN       =  0x49,
    ADB_MUTE              =  0x4a,
    ADB_BRIGHTNESS_DOWN   =  0x6b,
    ADB_BRIGHTNESS_UP     =  0x71,
    ADB_HOME              =  0x73,
    ADB_PAGE_UP           =  0x74,
    ADB_END               =  0x77,
    ADB_PAGE_DOWN         =  0x79,
    ADB_POWER             =  0x7f,
};

#endif /* message_h */
