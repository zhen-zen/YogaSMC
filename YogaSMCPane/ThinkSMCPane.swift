//
//  ThinkSMCPane.swift
//  YogaSMCPane
//
//  Created by Zhen on 12/9/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import AppKit
import Foundation

extension YogaSMCPane {
    @IBAction func vChargeThresholdStartSet(_ sender: NSTextFieldCell) {
        if let dict = getDictionary(thinkBatteryName[thinkBatteryNumber], service) {
            if let vStart = dict["BCTG"] as? NSNumber {
                var current = (vStart.intValue) & 0xff
                if current < 0 || current > 99 {
                    vChargeThresholdStart.isEnabled = false
                    return
                }
                if current == vChargeThresholdStart.integerValue { return }
                current = vChargeThresholdStart.integerValue
                #if DEBUG
                showOSD(String(format: "Start %d", current))
                #endif
                _ = sendNumber("setCMstart", current, service)
            }
        }
    }

    @IBAction func vChargeThresholdStopSet(_ sender: NSTextField) {
        if let dict = getDictionary(thinkBatteryName[thinkBatteryNumber], service) {
            if let vStop = dict["BCSG"] as? NSNumber {
                var current = (vStop.intValue) & 0xff
                if current < 0 || current > 99 {
                    vChargeThresholdStop.isEnabled = false
                    return
                }
                if current == 0 { current = 100 }
                if current == vChargeThresholdStop.integerValue { return }
                current = vChargeThresholdStop.integerValue
                #if DEBUG
                showOSD(String(format: "Stop %d", current))
                #endif
                _ = sendNumber("setCMstop", current == 100 ? 0 : current, service)
            }
        }
    }

    @IBAction func vPowerLEDSet(_ sender: NSSlider) {
        if !sendNumber("LED", thinkLEDCommand[vPowerLEDSlider.integerValue] + 0x00, service) {
            return
        }
    }
    @IBAction func vStandbyLEDSet(_ sender: NSSlider) {
        if !sendNumber("LED", thinkLEDCommand[vStandbyLEDSlider.integerValue] + 0x07, service) {
            return
        }
    }

    @IBAction func vThinkDotSet(_ sender: NSSlider) {
        if !sendNumber("LED", thinkLEDCommand[vThinkDotSlider.integerValue] + 0x0A, service) {
            return
        }
    }

    @IBAction func vCustomLEDSet(_ sender: NSSlider) {
        let value = thinkLEDCommand[vCustomLEDSlider.integerValue] + vCustomLEDList.indexOfSelectedItem
        #if DEBUG
        showOSD(String(format: "LED 0x%02X", value))
        #endif
        if !sendNumber("LED", value, service) {
            return
        }
    }

    @IBAction func vSecondFanSet(_ sender: NSButton) {
        defaults.setValue((vSecondFan.state == .on), forKey: "SecondThinkFan")
        _ = scriptHelper(reloadAS, "Reload YogaSMCNC")
    }
    @IBAction func vFanStopSet(_ sender: NSButton) {
        defaults.setValue((vFanStop.state == .on), forKey: "AllowFanStop")
        _ = scriptHelper(reloadAS, "Reload YogaSMCNC")
    }
    @IBAction func vDisableFanSet(_ sender: NSButton) {
        defaults.setValue((vDisableFan.state == .on), forKey: "DisableFan")
        _ = scriptHelper(reloadAS, "Reload YogaSMCNC")
    }

    func updateThinkBattery() -> Bool {
        _ = sendNumber("Battery", thinkBatteryNumber, service)
        if let dict = getDictionary(thinkBatteryName[thinkBatteryNumber], service),
           let vStart = dict["BCTG"] as? NSNumber,
           let vStop = dict["BCSG"] as? NSNumber {
            vChargeThresholdStart.isEnabled = true
            vChargeThresholdStop.isEnabled = true
            vChargeThresholdStart.integerValue = vStart.intValue & 0xff
            vChargeThresholdStop.integerValue = (vStop.intValue & 0xff) == 0 ? 100 : vStop.intValue & 0xff
            return true
        }
        thinkBatteryNumber += 1
        return false
    }

    func updateThinkFan() {
        var connect: io_connect_t = 0
        if kIOReturnSuccess == IOServiceOpen(service, mach_task_self_, 0, &connect),
           connect != 0 {
            if kIOReturnSuccess == IOConnectCallScalarMethod(connect, UInt32(kYSMCUCOpen), nil, 0, nil, nil) {
                var input: UInt64 = 0x84
                var outputSize = 2
                var output: [UInt8] = Array(repeating: 0, count: 2)
                if kIOReturnSuccess == IOConnectCallMethod(connect, UInt32(kYSMCUCReadEC), &input, 1, nil, 0, nil, nil, &output, &outputSize),
                   outputSize == 2 {
                    vFanSpeed.intValue = Int32(output[0]) | Int32(output[1]) << 8
                }
            }
            IOServiceClose(connect)
        }
    }

    func updateThink(_ props: NSDictionary) {
        while thinkBatteryNumber <= 2 {
            if updateThinkBattery() {
                break
            }
        }
        updateThinkFan()
        #if DEBUG
        if !getBoolean("Dual fan", service) {
            vSecondFan.isEnabled = true
            vSecondFan.state = defaults.bool(forKey: "SecondThinkFan") ? .on : .off
        }
        #endif
        vFanStop.state = defaults.bool(forKey: "AllowFanStop") ? .on : .off
        if getBoolean("LEDSupport", service) {
            vPowerLEDSlider.isEnabled = true
            vStandbyLEDSlider.isEnabled = true
            vThinkDotSlider.isEnabled = true
            vCustomLEDSlider.isEnabled = true
        }
        vDisableFan.state = defaults.bool(forKey: "DisableFan") ? .on : .off
    }
}
