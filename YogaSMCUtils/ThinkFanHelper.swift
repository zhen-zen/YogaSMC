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
    let menu: NSMenu
    let connect: io_connect_t
    let main: Bool
    let defaults = UserDefaults(suiteName: "org.zhen.YogaSMC")!
    var enable = false
    var single: Bool
    let fanPrompt: String

    let slider = NSSlider(frame: NSRect(x: 15, y: 5, width: 200, height: 30))
    let fanReading = NSTextField(frame: NSRect(x: 12, y: 30, width: 110, height: 30))
    let autoMode = NSButton(frame: NSRect(x: 125, y: 35, width: 45, height: 30))
    let fullMode = NSButton(frame: NSRect(x: 175, y: 35, width: 45, height: 30))

    var savedLevel: UInt8 = 0x80

    var fanLevel = "HFSP" // 0x2f
    var fanStatus: UInt64  = 0x2f
    var fanSelect: UInt64  = 0x31
    var fanSpeed: UInt64  = 0x84

    public init(_ menu: NSMenu, _ connect: io_connect_t, _ main: Bool, _ single: Bool) {
        self.menu = menu
        self.connect = connect
        self.main = main
        self.single = single

        enable = menu.items[2].title.hasSuffix("ThinkVPC")
        fanPrompt = NSLocalizedString(main ? "Main: %d rpm" : "Alt: %d rpm", comment: "")

        slider.maxValue = 7
        slider.minValue = defaults.bool(forKey: "AllowFanStop") ? 0 : 1
        slider.numberOfTickMarks = 8 - Int(slider.minValue)
        slider.target = self
        slider.action = #selector(sliderChanged)
        slider.allowsTickMarkValuesOnly = true
        slider.isContinuous = false

        fanReading.isEditable = false
        fanReading.isSelectable = false
        fanReading.isBezeled = false
        fanReading.drawsBackground = false
        fanReading.stringValue = String(format: fanPrompt, 12345)
        fanReading.font = menu.font

        autoMode.title = NSLocalizedString("Auto", comment: "")
        autoMode.bezelStyle = .texturedRounded
        autoMode.setButtonType(.onOff)
        autoMode.target = self
        autoMode.action = #selector(buttonChanged)

        fullMode.title = NSLocalizedString("Full", comment: "")
        fullMode.bezelStyle = .texturedRounded
        fullMode.setButtonType(.onOff)
        fullMode.target = self
        fullMode.action = #selector(buttonChanged)

        let view = NSView(frame: NSRect(x: 0, y: 0, width: slider.frame.width + 40, height: slider.frame.height + 40))
        view.addSubview(slider)
        view.addSubview(fanReading)
        view.addSubview(autoMode)
        view.addSubview(fullMode)

        let item = NSMenuItem()
        item.view = view
        menu.insertItem(item, at: 4)
    }

    @objc func buttonChanged(_ sender: NSButton) {
        if sender == autoMode {
            autoMode.state = .on
            fullMode.state = .off
            savedLevel = 0x84 // safety min speed 4
            slider.intValue = 4
        } else if sender == fullMode {
            autoMode.state = .off
            fullMode.state = .on
            savedLevel = 0x47 // safety min speed 7
            slider.intValue = 7
        } else {
            os_log("Unknown fan mode: %s", type: .error, sender.title)
            return
        }

        setFanLevel()
    }

    @objc func sliderChanged(_ sender: NSSlider) {
        autoMode.state = .off
        fullMode.state = .off
        savedLevel = UInt8(sender.intValue)

        setFanLevel()
    }

    func setFanLevel() {
        guard enable, switchFan(main) else { return }

        var input = [savedLevel]
        if kIOReturnSuccess != IOConnectCallMethod(connect, UInt32(kYSMCUCWriteEC),
                                                   &fanStatus, 1, &input, 1, nil, nil, nil, nil) {
            os_log("Write Fan Speed failed!", type: .fault)
            showOSD("WriteFanFail")
            enable = false
        }
    }

    @objc func update(_ updateLevel: Bool = false) {
        guard enable, connect != 0, switchFan(main) else { return }

        var output: [UInt8] = [0, 0]
        var outputSize = 2
        guard kIOReturnSuccess == IOConnectCallMethod(connect, UInt32(kYSMCUCReadEC),
                                                      &fanSpeed, 1, nil, 0, nil, nil, &output, &outputSize),
           outputSize == 2 else {
            os_log("Failed to access EC", type: .error)
            enable = false
            return
        }

        fanReading.stringValue = String(format: fanPrompt, Int32(output[0]) | Int32(output[1]) << 8)

        if !updateLevel { return }

        outputSize = 1
        guard kIOReturnSuccess == IOConnectCallMethod(connect, UInt32(kYSMCUCReadECName),
                                                      nil, 0, &fanLevel, 4, nil, nil, &output, &outputSize) else {
            showOSD("ReadFanFail")
            os_log("Failed to read fan level!", type: .error)
            enable = false
            return
        }

        if (output[0] & 0x40) != 0 {
            autoMode.state = .off
            fullMode.state = .on
            savedLevel = 0x47
            slider.intValue = 7
        } else if (output[0] & 0x80) != 0 {
            autoMode.state = .on
            fullMode.state = .off
            savedLevel = 0x84
            slider.intValue = 4
        } else {
            autoMode.state = .off
            fullMode.state = .off
            if output[0] > 7 {
                os_log("Unknown level 0x%02x", type: .error, output[0])
                savedLevel = 7
            } else {
                savedLevel = output[0]
            }
            slider.intValue = Int32(savedLevel)
        }
    }

    func switchFan(_ main: Bool) -> Bool {
        if single { return true }

        var current: [UInt8] = [0]
        var outputSize = 1
        guard kIOReturnSuccess == IOConnectCallMethod(connect, UInt32(kYSMCUCReadEC),
                                                      &fanSelect, 1, nil, 0, nil, nil, &current, &outputSize),
           outputSize == 1 else {
            os_log("Failed to read current fan", type: .error)
            enable = false
            return false
        }
        os_log("fan reg: 0x%x", type: .info, current[0])

        if (current[0] & 0x1) != 0 {
            if !main {
                os_log("Already selected second fan", type: .info)
                return true
            }
            current[0] &= 0xfe
        } else {
            if main {
                os_log("Already selected main fan", type: .info)
                return true
            }
            current[0] |= 0x1
        }

        guard kIOReturnSuccess == IOConnectCallMethod(connect, UInt32(kYSMCUCWriteEC),
                                                      &fanSelect, 1, &current, 1, nil, nil, nil, nil) else {
            os_log("Failed to select another fan", type: .error)
            enable = false
            return false
        }
        return true
    }
}
