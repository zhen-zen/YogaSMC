//
//  ThinkSMCPane.swift
//  YogaSMCPane
//
//  Created by Zhen on 12/9/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import AppKit
import Foundation
import os.log

extension YogaSMCPane {
    @IBAction func vChargeThresholdStartSet(_ sender: NSTextField) {
        let target = sender.integerValue
        #if DEBUG
        showOSD(String(format: "Start %d", target))
        #endif

        if thinkBatteryNumber != sender.tag, !updateThinkBatteryIndex(sender.tag) {
            os_log("Failed to update battery %d", type: .error, sender.tag)
            sender.isEnabled = false
            return
        }

        if target == sender.integerValue { return }
        _ = sendNumber("setCMstart", target, service)
        sender.integerValue = target
    }

    @IBAction func vChargeThresholdStopSet(_ sender: NSTextField) {
        let target = sender.integerValue

        if thinkBatteryNumber != sender.tag, !updateThinkBatteryIndex(sender.tag) {
            os_log("Failed to update battery %d", type: .error, sender.tag)
            return
        }

        if target == sender.integerValue { return }
        #if DEBUG
        showOSD(String(format: "Stop %d", target))
        #endif
        _ = sendNumber("setCMstop", target == 100 ? 0 : target, service)
        sender.integerValue = target
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

    @IBAction func vSaveFanLevelSet(_ sender: NSButton) {
        defaults.setValue((vSaveFanLevel.state == .on), forKey: "SaveFanLevel")
        _ = scriptHelper(reloadAS, "Reload YogaSMCNC")
    }

    @IBAction func vMuteLEDFixupSet(_ sender: NSButton) {
        defaults.setValue((vMuteLEDFixup.state == .on), forKey: "ThinkMuteLEDFixup")
        _ = scriptHelper(reloadAS, "Reload YogaSMCNC")
    }

    func updateThinkBatteryIndex(_ index: Int) -> Bool {
        if !sendNumber("Battery", index, service) { return false }

        switch index {
        case 0:
            return updateThinkBattery(vChargeThresholdStart, vChargeThresholdStop)
        case 1:
            return updateThinkBattery(vPrimaryChargeThresholdStart, vPrimaryChargeThresholdStop)
        case 2:
            return updateThinkBattery(vSecondaryChargeThresholdStart, vSecondaryChargeThresholdStop)
        default:
            return false
        }
    }

    func updateThinkBattery(_ startThreshold: NSTextField, _ stopThreshold: NSTextField) -> Bool {
        guard startThreshold.tag == stopThreshold.tag else { return false}
        let index = startThreshold.tag

        guard let dict = getDictionary(thinkBatteryName[index], service),
              let vStart = dict["BCTG"] as? NSNumber,
              let vStop = dict["BCSG"] as? NSNumber,
              vStart.int32Value >= 0,
              vStart.int32Value & 0xff < 100,
              vStop.int32Value >= 0,
              vStop.int32Value & 0xff < 100 else { return false }

        startThreshold.isEnabled = true
        stopThreshold.isEnabled = true
        startThreshold.integerValue = vStart.intValue & 0xff
        stopThreshold.integerValue = (vStop.intValue & 0xff) == 0 ? 100 : vStop.intValue & 0xff

        thinkBatteryNumber = index
        return true
    }

    func updateThink(_ props: NSDictionary) {
        _ = updateThinkBatteryIndex(0)
        _ = updateThinkBatteryIndex(1)
        _ = updateThinkBatteryIndex(2)

        if let val = props["FnlockMode"] as? Bool {
            vFnKeyRadio.title = "FnKey"
            if val {
                vFnKeyRadio.state = .on
            } else {
                vFxKeyRadio.state = .on
            }
        }
        #if DEBUG
        if let val = props["Dual fan"] as? Bool,
           val == true {
            vSecondFan.isEnabled = true
            vSecondFan.state = defaults.bool(forKey: "SecondThinkFan") ? .on : .off
        }
        #endif
        vFanStop.state = defaults.bool(forKey: "AllowFanStop") ? .on : .off
        if let val = props["LEDSupport"] as? Bool,
           val == true {
            vPowerLEDSlider.isEnabled = true
            vStandbyLEDSlider.isEnabled = true
            vThinkDotSlider.isEnabled = true
            vCustomLEDSlider.isEnabled = true
        }
        vDisableFan.state = defaults.bool(forKey: "DisableFan") ? .on : .off
        vSaveFanLevel.state = defaults.bool(forKey: "SaveFanLevel") ? .on : .off
        vMuteLEDFixup.state = defaults.bool(forKey: "ThinkMuteLEDFixup") ? .on : .off
    }
}
