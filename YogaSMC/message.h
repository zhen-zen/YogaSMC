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

#define PnpDeviceIdVPCIdea  "VPC2004"
#define PnpDeviceIdVPCThink "LEN0268"

#define kIOPMPowerOff              0
#define kIOPMNumberPowerStates     2

static IOPMPowerState IOPMPowerStates[kIOPMNumberPowerStates] = {
    {1, kIOPMPowerOff, kIOPMPowerOff, kIOPMPowerOff, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, kIOPMPowerOn, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
};

#define kDeliverNotifications   "RM,deliverNotifications"

enum
{
    // from keyboard to mouse/touchpad
    kSMC_setDisableTouchpad = iokit_vendor_specific_msg(100),   // set disable/enable touchpad (data is bool*)
    kSMC_getDisableTouchpad = iokit_vendor_specific_msg(101),   // get disable/enable touchpad (data is bool*)

    // from sensor to keyboard
    kSMC_setKeyboardStatus  = iokit_vendor_specific_msg(200),   // set disable/enable keyboard (data is bool*)
    kSMC_getKeyboardStatus  = iokit_vendor_specific_msg(201),   // get disable/enable keyboard (data is bool*)

    // SMC message types
    kSMC_VPCType            = iokit_vendor_specific_msg(500),   // get loaded VPC type (data is UInt32*)
    kSMC_YogaEvent          = iokit_vendor_specific_msg(501),   // Forward Yoga Event (data is UInt32*)
    kSMC_FnlockEvent        = iokit_vendor_specific_msg(502)    // Forward Fnlock Event
//    kSMC_PowerEvent         = iokit_vendor_specific_msg(503)
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
