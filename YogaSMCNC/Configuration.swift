//
//  Configuration.swift
//  YogaSMCNC
//
//  Created by Zhen on 10/17/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import Foundation
import os.log

enum eventAction : String {
    // Userspace
    case nothing, script
    case airplane, wireless, bluetooth, bluetoothdiscoverable
    case prefpane, spotlight, search, sleep, micmute
    case mission, launchpad, desktop, expose
    case mirror, camera
    // Driver
    case backlight, keyboard, thermal
}

struct eventDesc {
    let name : String
    let image : NSString?
    let action : eventAction
    let display : Bool
    let option : String?

    init(_ name: String, _ image: String? = nil, action: eventAction = .nothing, display: Bool = true, option: String? = nil) {
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
        self.action = action
        self.display = display
        self.option = option
    }

    init(_ name: String, _ image: eventImage, action: eventAction = .nothing, display: Bool = true, option: String? = nil) {
        self.name = name
        if let path = Bundle.main.path(forResource: image.rawValue, ofType: "pdf"),
           path.hasPrefix("/Applications") {
            self.image = path as NSString
        } else {
            self.image = nil
        }
        self.action = action
        self.display = display
        self.option = option
    }
}

let IdeaEvents : Dictionary<UInt32, Dictionary<UInt32, eventDesc>> = [
    0x00 : [0: eventDesc("Special Button", display: false),
            0x40: eventDesc("Fn-Q Cooling", action: .thermal)],
    0x01 : [0: eventDesc("Keyboard Backlight", action: .backlight, display: false)],
    0x02 : [0: eventDesc("Screen Off"),
            1: eventDesc("Screen On")],
    0x05 : [0: eventDesc("TouchPad Off"),
            1: eventDesc("TouchPad On")],
    0x07 : [0: eventDesc("Camera", action: .camera)],
    0x08 : [0: eventDesc("Mic Mute", action: .micmute, display: false)],
    0x0A : [0: eventDesc("TouchPad On", display: false)],
    0x0D : [0: eventDesc("Airplane Mode", action: .airplane)],
]

let ThinkEvents : Dictionary<UInt32, Dictionary<UInt32, eventDesc>> = [
    TP_HKEY_EV_SLEEP.rawValue : [0: eventDesc("Sleep", action: .sleep, display: false)], // 0x1004
    TP_HKEY_EV_NETWORK.rawValue : [0: eventDesc("Airplane Mode", action: .wireless)], // 0x1005
    TP_HKEY_EV_DISPLAY.rawValue : [0: eventDesc("Second Display", action: .mirror)], // 0x1007
    TP_HKEY_EV_BRGHT_UP.rawValue : [0: eventDesc("Brightness Up", display: false)], // 0x1010
    TP_HKEY_EV_BRGHT_DOWN.rawValue : [0: eventDesc("Brightness Down", display: false)], // 0x1011
    TP_HKEY_EV_KBD_LIGHT.rawValue : [0: eventDesc("Keyboard Backlight", action: .backlight, display: false)], // 0x1012
    TP_HKEY_EV_MIC_MUTE.rawValue : [0: eventDesc("Mic Mute", action: .micmute, display: false)], // 0x101B
    TP_HKEY_EV_SETTING.rawValue : [0: eventDesc("Settings", action: .prefpane, display: false)], // 0x101D
    TP_HKEY_EV_SEARCH.rawValue : [0: eventDesc("Search", action: .spotlight, display: false)], // 0x101E
    TP_HKEY_EV_MISSION.rawValue : [0: eventDesc("Mission Control", action: .mission, display: false)], // 0x101F
    TP_HKEY_EV_APPS.rawValue : [0: eventDesc("Launchpad", action: .launchpad, display: false)], // 0x1020
    TP_HKEY_EV_STAR.rawValue : [0: eventDesc("Custom Hotkey", .Star , action: .script, option: prefpaneAS)], // 0x1311
    TP_HKEY_EV_BLUETOOTH.rawValue : [0: eventDesc("Bluetooth", action: .bluetooth)], // 0x1314
    TP_HKEY_EV_KEYBOARD.rawValue : [0: eventDesc("Keyboard Disable", action: .keyboard)], // 0x1315
    TP_HKEY_EV_THM_TABLE_CHANGED.rawValue : [0: eventDesc("Thermal Table Change", display: false)], // 0x6030
    TP_HKEY_EV_AC_CHANGED.rawValue: [0: eventDesc("AC Status Change", display: false)], // 0x6040
    TP_HKEY_EV_KEY_FN_ESC.rawValue : [0: eventDesc("FnLock")], // 0x6060
]
