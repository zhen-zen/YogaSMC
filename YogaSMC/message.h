//
//  message.h
//  YogaSMC
//
//  Created by Zhen on 7/26/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef message_h
#define message_h

enum
{
    // from keyboard to mouse/touchpad
    kSMC_setDisableTouchpad = iokit_vendor_specific_msg(100),   // set disable/enable touchpad (data is bool*)
    kSMC_getDisableTouchpad = iokit_vendor_specific_msg(101),   // get disable/enable touchpad (data is bool*)

    // from sensor to keyboard
    kSMC_setKeyboardStatus  = iokit_vendor_specific_msg(200),   // set disable/enable keyboard (data is bool*)
    kSMC_getKeyboardStatus  = iokit_vendor_specific_msg(201),   // get disable/enable keyboard (data is bool*)

    // SMC message types
    kSMC_YogaEvent          = iokit_vendor_specific_msg(500),
    kSMC_FnlockEvent        = iokit_vendor_specific_msg(501),
    kSMC_PowerEvent         = iokit_vendor_specific_msg(502)
};

// from VoodooPS2
enum
{
    // from keyboard to mouse/touchpad
//    kPS2M_setDisableTouchpad = iokit_vendor_specific_msg(100),   // set disable/enable touchpad (data is bool*)
//    kPS2M_getDisableTouchpad = iokit_vendor_specific_msg(101),   // get disable/enable touchpad (data is bool*)
    kPS2M_notifyKeyPressed = iokit_vendor_specific_msg(102),     // notify of time key pressed (data is PS2KeyInfo*)

    kPS2M_notifyKeyTime = iokit_vendor_specific_msg(110),        // notify of timestamp a non-modifier key was pressed (data is uint64_t*)

    kPS2M_resetTouchpad = iokit_vendor_specific_msg(151),        // Force touchpad reset (data is int*)

    // from sensor (such as yoga mode indicator) to keyboard
//    kPS2K_setKeyboardStatus = iokit_vendor_specific_msg(200),   // set disable/enable keyboard (data is bool*)
//    kPS2K_getKeyboardStatus = iokit_vendor_specific_msg(201)    // get disable/enable keyboard (data is bool*)
};
#endif /* message_h */
