//
//  OSDHelper.swift
//  YogaSMC
//
//  Created by Zhen on 10/14/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import Foundation
import os.log

// from https://ffried.codes/2018/01/20/the-internals-of-the-macos-hud/
//@objc enum OSDImage: CLongLong {
//    case kBrightness = 1
//    case brightness2 = 2
//    case kVolume = 3
//    case kMute = 4
//    case volume5 = 5
//    case kEject = 6
//    case brightness7 = 7
//    case brightness8 = 8
//    case kAirportRange = 9
//    case wireless2Forbid = 10
//    case kBright = 11
//    case kBrightOff = 12
//    case kBright13 = 13
//    case kBrightOff14 = 14
//    case ajar = 15
//    case mute16 = 16
//    case volume17 = 17
//    case empty18 = 18
//    case kRemoteLinkedGeneric = 19
//    case kRemoteSleepGeneric = 20 // will put into sleep
//    case muteForbid = 21
//    case volumeForbid = 22
//    case volume23 = 23
//    case empty24 = 24
//    case kBright25 = 25
//    case kBrightOff26 = 26
//    case backlightonForbid = 27
//    case backlightoffForbid = 28
//    /* and more cases from 1 to 28 (except 18 and 24) */
//}

let defaultImage: NSString = "/System/Library/CoreServices/OSDUIHelper.app/Contents/Resources/kBrightOff.pdf"

// Bundled resources
enum EventImage: String {
    case kAirplaneMode, kAntenna, kMic, kMicOff, kKeyboard, kKeyboardOff, kWifi, kWifiOff
    case kBacklightHigh, kBacklightLow, kBacklightOff, kCapslockOn, kCapslockOff, kDock, kUndock
    case kBluetooth, kCamera, kFunctionKey, kFunctionKeyOff, kFunctionKeyOn, kSecondDisplay, kSleep, kStar
}

// from https://github.com/alin23/Lunar/blob/master/Lunar/Data/Hotkeys.swift
func showOSDRaw(_ prompt: String, _ img: NSString? = nil, duration: UInt32 = 1000, priority: UInt32 = 0x1f4) {
    guard let manager = OSDManager.sharedManager() as? OSDManager else {
        if #available(macOS 10.12, *) {
            os_log("OSDManager unavailable", type: .error)
        }
        return
    }

    manager.showImage(
        atPath: img ?? defaultImage,
        onDisplayID: CGMainDisplayID(),
        priority: priority,
        msecUntilFade: duration,
        withText: prompt as NSString)
}

func showOSD(_ prompt: String, _ img: NSString? = nil, duration: UInt32 = 1000, priority: UInt32 = 0x1f4) {
    showOSDRaw(NSLocalizedString(prompt, comment: "LocalizedString"), img, duration: duration, priority: priority)
}

func showOSDRes(_ prompt: String, _ image: EventImage, duration: UInt32 = 1000, priority: UInt32 = 0x1f4) {
    var img: NSString?
    if let path = Bundle.main.pathForImageResource(String(image.rawValue.dropFirst(1))),
              path.hasPrefix("/Applications") {
        img = path as NSString
    }
    showOSDRaw(NSLocalizedString(prompt, comment: "LocalizedString"), img, duration: duration, priority: priority)
}

func showOSDRes(
    _ prompt: String,
    _ status: String,
    _ image: EventImage,
    duration: UInt32 = 1000,
    priority: UInt32 = 0x1f4
) {
    var img: NSString?
    if let path = Bundle.main.pathForImageResource(String(image.rawValue.dropFirst(1))),
              path.hasPrefix("/Applications") {
        img = path as NSString
    }
    var alias = prompt
    if img != nil {
        alias = NSLocalizedString(prompt, comment: "")
    }
    if alias.isEmpty {
        showOSDRaw(alias, img, duration: duration, priority: priority)
    } else {
        alias += " " + NSLocalizedString(status, comment: "")
        showOSDRaw(alias, img, duration: duration, priority: priority)
    }
}
