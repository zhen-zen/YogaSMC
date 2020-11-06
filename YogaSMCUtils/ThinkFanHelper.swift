//
//  ThinkFanHelper.swift
//  YogaSMCNC
//
//  Created by Zhen on 11/5/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import AppKit
import Foundation
import os.log

class ThinkFanHelper {
    var appMenu: NSMenu
    var connect : io_connect_t
    let defaults = UserDefaults(suiteName: "org.zhen.YogaSMC")!

    var slider : NSSlider!
    var fanLevel: NSTextField!

    var secondThinkFan = false
    var ThinkFanSpeed = "HFSP" // 0x2f
//    var ThinkFanStatus : UInt64  = 0x2f
    var ThinkFanSelect : UInt64  = 0x31
    var ThinkFanRPM : UInt64  = 0x84

    public init(_ menu: NSMenu, _ connect: io_connect_t) {
        self.appMenu = menu
        self.connect = connect

        appMenu.insertItem(withTitle: "Fan", action: nil, keyEquivalent: "", at: 4)
        let item = NSMenuItem()
        if defaults.bool(forKey: "AllowFanStop") {
            slider = NSSlider(value: 0, minValue: 0, maxValue: 8, target: self, action: #selector(valueChanged))
            slider.numberOfTickMarks = 9
        } else {
            slider = NSSlider(value: 0, minValue: 1, maxValue: 8, target: self, action: #selector(valueChanged))
            slider.numberOfTickMarks = 8
        }
        slider.allowsTickMarkValuesOnly = true
        slider.isContinuous = false
        slider.frame.size.width = 180
        slider.frame.origin = NSPoint(x: 20, y: 5)
        let view = NSView(frame: NSRect(x: 0, y: 0, width: slider.frame.width + 50, height: slider.frame.height + 10))
        view.addSubview(slider)
        fanLevel = NSTextField(frame: NSRect(x: slider.frame.width + 25, y: 0, width: 30, height: slider.frame.height + 5))
        fanLevel.isEditable = false
        fanLevel.isSelectable = false
        fanLevel.isBezeled = false
        fanLevel.drawsBackground = false
        fanLevel.font = .systemFont(ofSize: 14)
        view.addSubview(fanLevel!)
        item.view = view
        appMenu.insertItem(item, at: 5)
        if defaults.bool(forKey: "SecondThinkFan") {
            secondThinkFan = true
            appMenu.insertItem(withTitle: "Fan2", action: nil, keyEquivalent: "", at: 6)
        }
        #if DEBUG
        appMenu.insertItem(withTitle: "HFNI", action: nil, keyEquivalent: "", at: secondThinkFan ? 7 : 6)
        #endif
//        if appMenu.items[7].title == "HFNI: 7" {
//            os_log("Might be auto mode at startup", type: .info)
//        }
    }

    @objc func valueChanged(_ sender: NSSlider) {
        fanLevel.integerValue = sender.integerValue == 8 ? 0x80 : sender.integerValue
        guard appMenu.items[2].title == "Class: ThinkVPC" else {
            showOSD("Val: \(sender.integerValue)")
            return
        }

        var addr : UInt64 = 0x2f // HFSP
        var input : [UInt8] = [UInt8(fanLevel.integerValue)]
        if kIOReturnSuccess != IOConnectCallMethod(connect, UInt32(kYSMCUCWriteEC), &addr, 1, &input, 1, nil, nil, nil, nil) {
            os_log("Write Fan Speed failed!", type: .fault)
            showOSD("Write Fan Speed failed!")
        }
    }

    @objc func update() {
        guard connect != 0 else {
            return
        }

        var outputSize = 2
        var output : [UInt8] = [0, 0]
        guard kIOReturnSuccess == IOConnectCallMethod(connect, UInt32(kYSMCUCReadEC), &ThinkFanRPM, 1, nil, 0, nil, nil, &output, &outputSize),
           outputSize == 2 else {
            os_log("Failed to access EC", type: .error)
            return
        }

        appMenu.items[4].title = "Fan: \(Int32(output[0]) | Int32(output[1]) << 8) rpm"

        outputSize = 1

        if kIOReturnSuccess == IOConnectCallMethod(connect, UInt32(kYSMCUCReadECName), nil, 0, &ThinkFanSpeed, 4, nil, nil, &output, &outputSize) {
            slider.integerValue = (output[0] == 0x80 ? Int(output[0]) : 8)
            fanLevel.integerValue = Int(output[0])
        } else {
            fanLevel.stringValue = "?"
        }

        #if DEBUG
        var name = "HFNI" // 0x83
        if kIOReturnSuccess == IOConnectCallMethod(connect, UInt32(kYSMCUCReadECName), nil, 0, &name, 4, nil, nil, &output, &outputSize) {
            appMenu.items[secondThinkFan ? 7 : 6].title = "HFNI: \(output[0])"
        }
        #endif

        if !secondThinkFan {
            return
        }

        guard kIOReturnSuccess == IOConnectCallMethod(connect, UInt32(kYSMCUCReadEC), &ThinkFanSelect, 1, nil, 0, nil, nil, &output, &outputSize),
           outputSize == 1 else {
            os_log("Failed to read current fan", type: .error)
            return
        }
        os_log("2nd fan reg: 0x%x", type: .info, output[0])

        var input : [UInt8] = [0];
        if (output[0] & 0x1) != 0 {
            input[0] = output[0] & 0xfe
        } else {
            input[0] = output[0] | 0x1
        }

        guard kIOReturnSuccess == IOConnectCallMethod(connect, UInt32(kYSMCUCWriteEC), &ThinkFanSelect, 1, &input, 1, nil, nil, nil, nil) else {
            os_log("Failed to select second fan", type: .error)
            secondThinkFan = false
            return
        }

        outputSize = 2
        if kIOReturnSuccess == IOConnectCallMethod(connect, UInt32(kYSMCUCReadEC), &ThinkFanRPM, 1, nil, 0, nil, nil, &output, &outputSize),
           outputSize == 2 {
            appMenu.items[6].title = "Fan2: \(Int32(output[0]) | Int32(output[1]) << 8) rpm"
        } else {
            os_log("Failed to access second fan", type: .error)
            secondThinkFan = false
        }

        if (input[0] & 0x1) != 0 {
            input[0] &= 0xfe
        } else {
            input[0] |= 0x1
        }

        guard kIOReturnSuccess == IOConnectCallMethod(connect, UInt32(kYSMCUCWriteEC), &ThinkFanSelect, 1, &input, 1, nil, nil, nil, nil) else {
            os_log("Failed to select first fan", type: .error)
            secondThinkFan = false
            return
        }
    }
}
