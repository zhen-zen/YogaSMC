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

    let slider = NSSlider()
    let fanLevel = NSTextField()
    let autoMode = NSButton()
    let fullMode = NSButton()

    var secondThinkFan = false
    var ThinkFanSpeed = "HFSP" // 0x2f
    var ThinkFanStatus : UInt64  = 0x2f
    var ThinkFanSelect : UInt64  = 0x31
    var ThinkFanRPM : UInt64  = 0x84

    public init(_ menu: NSMenu, _ connect: io_connect_t) {
        self.appMenu = menu
        self.connect = connect

        slider.frame = NSRect(x: 20, y: 5, width: 200, height: 30)
        slider.maxValue = 7
        if defaults.bool(forKey: "AllowFanStop") {
            slider.minValue = 0
            slider.numberOfTickMarks = 8
        } else {
            slider.minValue = 1
            slider.numberOfTickMarks = 7
        }
        slider.target = self
        slider.action = #selector(sliderChanged)
        slider.allowsTickMarkValuesOnly = true
        slider.isContinuous = false

        fanLevel.frame = NSRect(x: 22, y: 30, width: 100, height: 30)
        fanLevel.isEditable = false
        fanLevel.isSelectable = false
        fanLevel.isBezeled = false
        fanLevel.drawsBackground = false
        fanLevel.stringValue = "Fan: 1234 rpm"

        autoMode.frame = NSRect(x: 125, y: 35, width: 45, height: 30)
        autoMode.title = "Auto"
        autoMode.bezelStyle = .texturedRounded
        autoMode.setButtonType(.onOff)
        autoMode.target = self
        autoMode.action = #selector(buttonChanged)

        fullMode.frame = NSRect(x: 175, y: 35, width: 45, height: 30)
        fullMode.title = "Full"
        fullMode.bezelStyle = .texturedRounded
        fullMode.setButtonType(.onOff)
        fullMode.target = self
        fullMode.action = #selector(buttonChanged)

        let view = NSView(frame: NSRect(x: 0, y: 0, width: slider.frame.width + 20, height: slider.frame.height + 40))
        view.addSubview(slider)
        view.addSubview(fanLevel)
        view.addSubview(autoMode)
        view.addSubview(fullMode)

        let item = NSMenuItem()
        item.view = view
        appMenu.insertItem(item, at: 4)

        if defaults.bool(forKey: "SecondThinkFan") {
            secondThinkFan = true
            appMenu.insertItem(withTitle: "Fan2", action: nil, keyEquivalent: "", at: 5)
        }
        #if DEBUG
        appMenu.insertItem(withTitle: "HFNI", action: nil, keyEquivalent: "", at: secondThinkFan ? 6 : 5)
        #endif
    }

    @objc func buttonChanged(_ sender: NSButton) {
        var input : [UInt8] = [0]
        if (sender == autoMode) {
            fullMode.state = .off
            input[0] = 0x84 // safety min speed 4
        } else if (sender == fullMode) {
            autoMode.state = .off
            input[0] = 0x47 // safety min speed 7
        }

        guard appMenu.items[2].title == "Class: ThinkVPC" else {
            showOSD("Val: \(sender.title)")
            return
        }

        if kIOReturnSuccess != IOConnectCallMethod(connect, UInt32(kYSMCUCWriteEC), &ThinkFanStatus, 1, &input, 1, nil, nil, nil, nil) {
            os_log("Write Fan Speed failed!", type: .fault)
            showOSD("Write Fan Speed failed!")
        }
    }

    @objc func sliderChanged(_ sender: NSSlider) {
        autoMode.state = .off
        fullMode.state = .off
        guard appMenu.items[2].title == "Class: ThinkVPC" else {
            showOSD("Val: \(sender.intValue)")
            return
        }

        var input : [UInt8] = [UInt8(sender.intValue)]
        if kIOReturnSuccess != IOConnectCallMethod(connect, UInt32(kYSMCUCWriteEC), &ThinkFanStatus, 1, &input, 1, nil, nil, nil, nil) {
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

        fanLevel.stringValue = "Fan: \(Int32(output[0]) | Int32(output[1]) << 8) rpm"

        outputSize = 1

        if kIOReturnSuccess == IOConnectCallMethod(connect, UInt32(kYSMCUCReadECName), nil, 0, &ThinkFanSpeed, 4, nil, nil, &output, &outputSize) {
            if ((output[0] & 0x40) != 0) {
                autoMode.state = .off
                fullMode.state = .on
            } else if ((output[0] & 0x80) != 0) {
                autoMode.state = .on
                fullMode.state = .off
            } else {
                autoMode.state = .off
                fullMode.state = .off
                if (output[0] > 7) {
                    os_log("Unknown level 0x%02x", type: .error, output[0])
                    slider.intValue = 7
                } else {
                    slider.intValue = Int32(output[0])
                }
            }
        } else {
            showOSD("Failed to read fan level!")
            os_log("Failed to read fan level!", type: .error)
            return
        }

        #if DEBUG
        var name = "HFNI" // 0x83
        if kIOReturnSuccess == IOConnectCallMethod(connect, UInt32(kYSMCUCReadECName), nil, 0, &name, 4, nil, nil, &output, &outputSize) {
            appMenu.items[secondThinkFan ? 6 : 5].title = "HFNI: \(output[0])"
            os_log("HFNI: %d", type: .info, output[0])
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
            appMenu.items[5].title = "Fan2: \(Int32(output[0]) | Int32(output[1]) << 8) rpm"
        } else {
            os_log("Failed to access second fan", type: .error)
            secondThinkFan = false
        }

        #if DEBUG
        if kIOReturnSuccess == IOConnectCallMethod(connect, UInt32(kYSMCUCReadECName), nil, 0, &name, 4, nil, nil, &output, &outputSize) {
            os_log("HFNI?: %d", type: .info, output[0])
        }
        #endif

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
