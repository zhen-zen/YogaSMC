//
//  Configuration.swift
//  YogaSMCNC
//
//  Created by Zhen on 10/17/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import Foundation
import os.log

enum EventAction: String {
    // Userspace
    case nothing, script
    case airplane, wireless, bluetooth, bluetoothdiscoverable
    case prefpane, spotlight, search, siri, sleep, micmute
    case mission, launchpad, desktop, expose
    case mirror, camera, yoga
    // Driver
    case backlight, keyboard, thermal
}

struct EventDesc {
    let name: String
    let image: NSString?
    let action: EventAction
    let display: Bool
    let option: String?

    init(_ name: String, _ image: String? = nil, act: EventAction = .nothing, display: Bool = true, opt: String? = nil) {
        self.name = name
        if let img = image {
            if img.hasPrefix("/") {
                self.image = img as NSString
            } else if let path = Bundle.main.path(forResource: img, ofType: nil),
                      path.hasPrefix("/Applications") {
                self.image = path as NSString
            } else {
                os_log("Incompatible res path: %s", type: .error, img)
                self.image = nil
            }
        } else {
            self.image = nil
        }
        self.action = act
        self.display = display
        self.option = opt
    }

    init(_ name: String, _ image: EventImage, act: EventAction = .nothing, display: Bool = true, opt: String? = nil) {
        self.name = name
        if let path = Bundle.main.path(forResource: image.rawValue, ofType: "pdf"),
           path.hasPrefix("/Applications") {
            self.image = path as NSString
        } else {
            self.image = nil
        }
        self.action = act
        self.display = display
        self.option = opt
    }
}

let ideaEvents: [UInt32: [UInt32: EventDesc]] = [
    0x00: [0: EventDesc("Special Button", display: false),
            0x40: EventDesc("Fn-Q Cooling", act: .thermal)],
    0x01: [0: EventDesc("Keyboard Backlight", act: .backlight, display: false)],
    0x02: [0: EventDesc("Screen Off"),
            1: EventDesc("Screen On")],
    0x05: [0: EventDesc("TouchPad Off"),
            1: EventDesc("TouchPad On")],
    0x06: [0: EventDesc("Switch Video")],
    0x07: [0: EventDesc("Camera", .Camera, act: .camera)],
    0x08: [0: EventDesc("Mic Mute", act: .micmute, display: false)],
    0x0A: [0: EventDesc("TouchPad On", display: false)],
    0x0D: [0: EventDesc("Airplane Mode", act: .airplane)],
    0x10: [0: EventDesc("Yoga Mode", act: .yoga),
            1: EventDesc("Laptop Mode"),
            2: EventDesc("Tablet Mode"),
            3: EventDesc("Stand Mode"),
            4: EventDesc("Tent Mode")],
    0x11: [0: EventDesc("FnKey Disabled", .FunctionKey),
            1: EventDesc("FnKey Enabled", .FunctionKey)]
]

let thinkEvents: [UInt32: [UInt32: EventDesc]] = [
    TP_HKEY_EV_SLEEP.rawValue: [0: EventDesc("Sleep", act: .sleep, display: false)], // 0x1004
    TP_HKEY_EV_NETWORK.rawValue: [0: EventDesc("Airplane Mode", act: .wireless)], // 0x1005
    TP_HKEY_EV_DISPLAY.rawValue: [0: EventDesc("Second Display", .SecondDisplay, act: .mirror)], // 0x1007
    TP_HKEY_EV_BRGHT_UP.rawValue: [0: EventDesc("Brightness Up", display: false)], // 0x1010
    TP_HKEY_EV_BRGHT_DOWN.rawValue: [0: EventDesc("Brightness Down", display: false)], // 0x1011
    TP_HKEY_EV_KBD_LIGHT.rawValue: [0: EventDesc("Keyboard Backlight", act: .backlight, display: false)], // 0x1012
    TP_HKEY_EV_MIC_MUTE.rawValue: [0: EventDesc("Mic Mute", act: .micmute, display: false)], // 0x101B
    TP_HKEY_EV_SETTING.rawValue: [0: EventDesc("Settings", act: .prefpane, display: false)], // 0x101D
    TP_HKEY_EV_SEARCH.rawValue: [0: EventDesc("Search", act: .siri, display: false)], // 0x101E
    TP_HKEY_EV_MISSION.rawValue: [0: EventDesc("Mission Control", act: .mission, display: false)], // 0x101F
    TP_HKEY_EV_APPS.rawValue: [0: EventDesc("Launchpad", act: .launchpad, display: false)], // 0x1020
    TP_HKEY_EV_STAR.rawValue: [0: EventDesc("Custom Hotkey", .Star, act: .script, opt: prefpaneAS)], // 0x1311
    TP_HKEY_EV_BLUETOOTH.rawValue: [0: EventDesc("Bluetooth", act: .bluetooth)], // 0x1314
    TP_HKEY_EV_KEYBOARD.rawValue: [0: EventDesc("Keyboard Disabled", .KeyboardOff),
                                    1: EventDesc("Keyboard Enabled", .Keyboard)], // 0x1315
    TP_HKEY_EV_LID_CLOSE.rawValue: [0: EventDesc("LID Close", display: false)], // 0x5001
    TP_HKEY_EV_LID_OPEN.rawValue: [0: EventDesc("LID Open", display: false)], // 0x5002
    TP_HKEY_EV_THM_TABLE_CHANGED.rawValue: [0: EventDesc("Thermal Table Change", display: false)], // 0x6030
    TP_HKEY_EV_THM_CSM_COMPLETED.rawValue: [0: EventDesc("Thermal Control Change", display: false)], // 0x6032
    TP_HKEY_EV_AC_CHANGED.rawValue: [0: EventDesc("AC Status Change", display: false)], // 0x6040
    TP_HKEY_EV_BACKLIGHT_CHANGED.rawValue: [0: EventDesc("Backlight Changed", display: false)], // 0x6050
    TP_HKEY_EV_KEY_FN_ESC.rawValue: [0: EventDesc("FnLock", .FunctionKey)], // 0x6060
    TP_HKEY_EV_PALM_DETECTED.rawValue: [0: EventDesc("Palm Detected", display: false)], // 0x60B0
    TP_HKEY_EV_PALM_UNDETECTED.rawValue: [0: EventDesc("Palm Undetected", display: false)], // 0x60B1
    TP_HKEY_EV_THM_TRANSFM_CHANGED.rawValue: [0: EventDesc("Tablet Mode", display: false)] // 0x60F0
]

let HIDDEvents: [UInt32: [UInt32: EventDesc]] = [
    0x08: [0: EventDesc("Airplane Mode", act: .airplane)],
    0x0B: [0: EventDesc("Sleep", act: .sleep, display: false)],
    0x0E: [0: EventDesc("STOPCD")],
    0xC8: [0: EventDesc("Rotate Lock")], // Down
    0xC9: [0: EventDesc("Rotate Lock", display: false)], // Up
    0xCC: [0: EventDesc("Convertible")], // Down
    0xCD: [0: EventDesc("Convertible", display: false)] // Up
]
