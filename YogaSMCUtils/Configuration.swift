//
//  Configuration.swift
//  YogaSMCNC
//
//  Created by Zhen on 10/17/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import AppKit
import Foundation
import os.log

enum EventAction: String {
    // Userspace
    case nothing, script, launchapp, launchbundle
    case airplane, wireless, bluetooth, bluetoothdiscoverable
    case prefpane, spotlight, search, siri, sleep, micmute
    case mission, launchpad, desktop, expose
    case mirror, camera, yoga
    // Driver
    case backlight, fnlock, keyboard, thermal
}

struct EventDesc {
    let name: String
    let image: NSString?
    let action: EventAction
    let display: Bool
    let option: String?

    init(_ name: String, _ img: String? = nil, act: EventAction = .nothing, display: Bool = true, opt: String? = nil) {
        self.name = name
        if let path = img {
            if path.hasPrefix("/") {
                self.image = path as NSString
            } else if let res = Bundle.main.path(forResource: path, ofType: nil),
                      res.hasPrefix("/Applications") {
                self.image = res as NSString
            } else {
                if #available(macOS 10.12, *) {
                    os_log("Incompatible res path: %s", type: .error, path)
                }
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
        if let path = Bundle.main.path(forResource: String(image.rawValue.dropFirst(1)), ofType: "pdf"),
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

struct SharedConfig {
    var connect: io_connect_t = 0
    var events: [UInt32: [UInt32: EventDesc]] = [:]
    var modifier = NSEvent.ModifierFlags()
    var service: io_service_t = 0
}

private let commandFlag = UInt32(NSEvent.ModifierFlags.command.rawValue)
private let controlFlag = UInt32(NSEvent.ModifierFlags.control.rawValue)
private let optionFlag = UInt32(NSEvent.ModifierFlags.option.rawValue)

let ideaEvents: [UInt32: [UInt32: EventDesc]] = [
    0x00: [
        0x00: EventDesc("Special Button", display: false),
        0x01: EventDesc("Fn-Q Cooling", act: .thermal),
        0x02: EventDesc("OneKey Theater", .kStar, act: .script, opt: prefpaneAS),
        0x40: EventDesc("Fn-Q Cooling", act: .thermal)
    ],
    0x01: [0: EventDesc("Keyboard Backlight", act: .backlight)],
    0x02: [
        0: EventDesc("Screen Off"),
        1: EventDesc("Screen On")
    ],
    0x04: [0: EventDesc("Backlight Changed", display: false)],
    0x05: [
        0: EventDesc("TouchPad Off"),
        1: EventDesc("TouchPad On")
    ],
    0x06: [0: EventDesc("Switch Video")],
    0x07: [
        0: EventDesc("Camera", .kCamera, act: .camera),
        optionFlag: EventDesc("Photo Booth", act: .launchapp, display: false, opt: "Photo Booth")
    ],
    0x08: [0: EventDesc("Mic Mute", act: .micmute)],
    0x0A: [0: EventDesc("TouchPad On", display: false)],
    0x0C: [0: EventDesc("Keyboard Backlight", act: .backlight, display: false)],
    0x0D: [
        0: EventDesc("Airplane Mode", act: .airplane),
        controlFlag: EventDesc("Bluetooth", act: .bluetooth),
        optionFlag: EventDesc("BT Discoverable", act: .bluetoothdiscoverable),
        commandFlag: EventDesc("Wireless", act: .wireless)
    ],
    0x10: [
        0: EventDesc("Yoga Mode", act: .yoga),
        1: EventDesc("Laptop Mode"),
        2: EventDesc("Tablet Mode"),
        3: EventDesc("Stand Mode"),
        4: EventDesc("Tent Mode")
    ],
    0x11: [0: EventDesc("FnLock", act: .fnlock)],
    0x12: [
        1: EventDesc("Quiet"),
        2: EventDesc("Balance"),
        3: EventDesc("Performance")
    ]
]

let thinkEvents: [UInt32: [UInt32: EventDesc]] = [
    TP_HKEY_EV_SLEEP.rawValue: [
        0: EventDesc("Sleep", act: .sleep, display: false),
        optionFlag: EventDesc("Sleep", act: .sleep)
    ], // 0x1004
    TP_HKEY_EV_NETWORK.rawValue: [
        0: EventDesc("Wireless", act: .wireless),
        optionFlag: EventDesc("Airplane Mode", act: .airplane)
    ], // 0x1005
    TP_HKEY_EV_DISPLAY.rawValue: [0: EventDesc("Second Display", .kSecondDisplay, act: .mirror)], // 0x1007
    TP_HKEY_EV_BRGHT_UP.rawValue: [0: EventDesc("Brightness Up", display: false)], // 0x1010
    TP_HKEY_EV_BRGHT_DOWN.rawValue: [0: EventDesc("Brightness Down", display: false)], // 0x1011
    TP_HKEY_EV_KBD_LIGHT.rawValue: [0: EventDesc("Keyboard Backlight", act: .backlight)], // 0x1012
    TP_HKEY_EV_MIC_MUTE.rawValue: [0: EventDesc("Mic Mute", act: .micmute)], // 0x101B
    TP_HKEY_EV_SETTING.rawValue: [0: EventDesc("Settings", act: .prefpane, display: false)], // 0x101D
    TP_HKEY_EV_SEARCH.rawValue: [0: EventDesc("Search", act: .siri, display: false)], // 0x101E
    TP_HKEY_EV_MISSION.rawValue: [0: EventDesc("Mission Control", act: .mission, display: false)], // 0x101F
    TP_HKEY_EV_APPS.rawValue: [0: EventDesc("Launchpad", act: .launchpad, display: false)], // 0x1020
    TP_HKEY_EV_STAR.rawValue: [
        0: EventDesc("Custom Hotkey", .kStar, act: .script, opt: prefpaneAS),
        optionFlag: EventDesc("System Prefereces", .kStar, act: .launchbundle, opt: "com.apple.systempreferences")
    ], // 0x1311
    TP_HKEY_EV_BLUETOOTH.rawValue: [
        0: EventDesc("Bluetooth", act: .bluetooth),
        optionFlag: EventDesc("BT Discoverable", act: .bluetoothdiscoverable)
    ], // 0x1314
    TP_HKEY_EV_KEYBOARD.rawValue: [0: EventDesc("Keyboard Toggle", act: .keyboard)], // 0x1315
    TP_HKEY_EV_HOTPLUG_DOCK.rawValue: [0: EventDesc("Dock Attached", .kDock, display: true)], // 0x4010
    TP_HKEY_EV_HOTPLUG_UNDOCK.rawValue: [0: EventDesc("Dock Detached", .kUndock, display: true)], // 0x4011
    TP_HKEY_EV_LID_CLOSE.rawValue: [0: EventDesc("LID Close", display: false)], // 0x5001
    TP_HKEY_EV_LID_OPEN.rawValue: [0: EventDesc("LID Open", display: false)], // 0x5002
    TP_HKEY_EV_THM_TABLE_CHANGED.rawValue: [0: EventDesc("Thermal Table Change", display: false)], // 0x6030
    TP_HKEY_EV_THM_CSM_COMPLETED.rawValue: [0: EventDesc("Thermal Control Change", display: false)], // 0x6032
    TP_HKEY_EV_AC_CHANGED.rawValue: [0: EventDesc("AC Status Change", display: false)], // 0x6040
    TP_HKEY_EV_BACKLIGHT_CHANGED.rawValue: [0: EventDesc("Backlight Changed", display: false)], // 0x6050
    TP_HKEY_EV_KEY_FN_ESC.rawValue: [0: EventDesc("FnLock", act: .fnlock)], // 0x6060
    TP_HKEY_EV_PALM_DETECTED.rawValue: [0: EventDesc("Palm Detected", display: false)], // 0x60B0
    TP_HKEY_EV_PALM_UNDETECTED.rawValue: [0: EventDesc("Palm Undetected", display: false)], // 0x60B1
    TP_HKEY_EV_TABLET_CHANGED.rawValue: [
        0: EventDesc("Yoga Mode", act: .yoga),
        1: EventDesc("Laptop Mode"),
        2: EventDesc("Flat Mode"),
        3: EventDesc("Tablet Mode"),
        4: EventDesc("Stand Mode"),
        5: EventDesc("Tent Mode")
    ], // 0x60C0
    TP_HKEY_EV_THM_TRANSFM_CHANGED.rawValue: [0: EventDesc("Thermal Changed", display: false)] // 0x60F0
]

let HIDDEvents: [UInt32: [UInt32: EventDesc]] = [
    0x08: [
        0: EventDesc("Airplane Mode", act: .airplane),
        controlFlag: EventDesc("Bluetooth", act: .bluetooth),
        optionFlag: EventDesc("BT Discoverable", act: .bluetoothdiscoverable),
        commandFlag: EventDesc("Wireless", act: .wireless)
    ],
    0x0B: [
        0: EventDesc("Sleep", act: .sleep, display: false),
        optionFlag: EventDesc("Sleep", act: .sleep)
    ],
    0x0E: [0: EventDesc("STOPCD")],
    0xC8: [0: EventDesc("Rotate Lock")], // Down
    0xC9: [0: EventDesc("Rotate Lock", display: false)], // Up
    0xCC: [0: EventDesc("Convertible")], // Down
    0xCD: [0: EventDesc("Convertible", display: false)] // Up
]

func loadEvents(_ conf: inout SharedConfig) {
    guard let dict = UserDefaults(suiteName: "org.zhen.YogaSMC")?.object(forKey: "Events") as? [String: Any] else {
        if #available(macOS 10.12, *) {
            os_log("Events not found in preference, loading defaults", type: .info)
        }
        return
    }

    for (sid, entries) in dict {
        if let eid = UInt32(sid, radix: 16),
           let events = entries as? [String: Any] {
            var info = conf.events[eid] ?? [:]
            for (sdata, entry) in events {
                if let data = UInt32(sdata, radix: 16),
                   let event = entry as? [String: Any],
                   let name = event["name"] as? String,
                   let action = event["action"] as? String {
                    let opt = event["option"] as? String
                    if info[data] != nil,
                       action == "nothing",
                       (opt == "Unknown" || (opt == nil && name.hasPrefix("Event 0x"))) {
                        if #available(macOS 10.12, *) {
                            os_log("Override unknown event 0x%04x : %d", type: .info, eid, data)
                        }
                    } else {
                        info[data] = EventDesc(
                            name,
                            event["image"] as? String,
                            act: EventAction(rawValue: action) ?? .nothing,
                            display: event["display"] as? Bool ?? true,
                            opt: opt)
                    }
                } else {
                    if #available(macOS 10.12, *) {
                        os_log("Invalid event for 0x%04x : %s!", type: .info, eid, sdata)
                    }
                }
            }
            conf.events[eid] = info
        } else {
            if #available(macOS 10.12, *) {
                os_log("Invalid event %s!", type: .info, sid)
            }
        }
    }
    if #available(macOS 10.12, *) {
        os_log("Loaded %d events", type: .info, conf.events.count)
    }
}

func saveEvents(_ conf: inout SharedConfig) {
    var dict: [String: Any] = [:]
    for (eid, info) in conf.events {
        var events: [String: Any] = [:]
        for (data, desc) in info {
            var event: [String: Any] = [:]
            event["name"] = desc.name
            if let res = desc.image {
                if let path = Bundle.main.resourcePath,
                   res.hasPrefix(path) {
                    event["image"] = res.lastPathComponent
               } else {
                    event["image"] = res
               }
            }
            event["action"] = desc.action.rawValue
            event["display"] = desc.display
            if desc.option != nil {
                event["option"] = desc.option
            }
            events[String(data, radix: 16)] = event
        }
        dict[String(eid, radix: 16)] = events
    }
    UserDefaults(suiteName: "org.zhen.YogaSMC")?.setValue(dict, forKey: "Events")
}
