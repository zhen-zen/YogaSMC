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

#define kIOACPIMessageD0 0xd0
#define kIOACPIMessageReserved 0x80

#define PnpDeviceIdEC       "PNP0C09"
#define PnpDeviceIdVPCIdea  "VPC2004"
#define PnpDeviceIdVPCThink "LEN0268"

#define kIOPMPowerOff              0
#define kIOPMNumberPowerStates     2

static IOPMPowerState IOPMPowerStates[kIOPMNumberPowerStates] = {
    {1, kIOPMPowerOff, kIOPMPowerOff, kIOPMPowerOff, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, kIOPMPowerOn, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
};

#define kDeliverNotifications   "RM,deliverNotifications"

#define autoBacklightPrompt "AutoBacklight"
#define batteryPrompt "Battery"
#define backlightPrompt "BacklightLevel"
#define beepPrompt "Beep"
#define SSTPrompt "SST"
#define conservationPrompt "ConservationMode"
#define clamshellPrompt "ClamshellMode"
#define DYTCPrompt "DYTCMode"
#define DYTCFuncPrompt "DYTCFuncMode"
#define DYTCPerfPrompt "DYTCPerfMode"
#define ECLockPrompt "ECLock"
#define FnKeyPrompt "FnlockMode"
#define fanControlPrompt "FanControl"
#define KeyboardPrompt "KeyboardMode"
#define HotKeyPrompt "HotKey"
#define LEDPrompt "LED"
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

#define initFailure "%s evaluation failed, exiting"
#define updateFailure "%s evaluation failed"
#define updateSuccess "%s 0x%x"
#define toggleFailure "%s toggle failed"
#define toggleSuccess "%s set to 0x%x: %s"

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

    // SMC message types
    kSMC_VPCType            = iokit_vendor_specific_msg(500),   // set loaded VPC type (data is UInt32*)
    kSMC_YogaEvent          = iokit_vendor_specific_msg(501),   // set Yoga mode (data is UInt32*)
    kSMC_FnlockEvent        = iokit_vendor_specific_msg(502),   // notify Fnlock event
    kSMC_getConservation    = iokit_vendor_specific_msg(503),   // get conservation mode (data is bool*)
    kSMC_setConservation    = iokit_vendor_specific_msg(504)    // set conservation mode (data is bool*)
};

// from VoodooPS2
enum
{
    // from keyboard to mouse/touchpad
    kPS2M_notifyKeyPressed = iokit_vendor_specific_msg(102),     // notify of time key pressed (data is PS2KeyInfo*)

    kPS2M_notifyKeyTime = iokit_vendor_specific_msg(110),        // notify of timestamp a non-modifier key was pressed (data is uint64_t*)

    kPS2M_resetTouchpad = iokit_vendor_specific_msg(151),        // Force touchpad reset (data is int*)
};
#endif /* message_h */
