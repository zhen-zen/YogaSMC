//
//  YogaSMCPane.swift
//  YogaSMCPane
//
//  Created by Zhen on 9/17/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import AppKit
import Foundation
import IOKit
import PreferencePanes
import os.log

let DYTCCommand = ["L", "M", "H"]
let thinkLEDCommand = [0, 0x80, 0xA0, 0xC0]
let thinkBatteryName = ["BAT_ANY", "BAT_PRIMARY", "BAT_SECONDARY"]

class YogaSMCPane: NSPreferencePane {
    let service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("YogaVPC"))
    let defaults = UserDefaults(suiteName: "org.zhen.YogaSMC")!

    var thinkBatteryNumber = 0

    @IBOutlet weak var vVersion: NSTextField!
    @IBOutlet weak var vClass: NSTextField!
    @IBOutlet weak var vECRead: NSTextField!
    @IBOutlet weak var vHideMenubarIcon: NSButton!
    @IBAction func toggleMenubarIcon(_ sender: NSButton) {
        if vHideMenubarIcon.state == .on {
            defaults.setValue(true, forKey: "HideIcon")
            vMenubarIcon.isEnabled = false
        } else {
            defaults.setValue(false, forKey: "HideIcon")
            vMenubarIcon.isEnabled = true
        }
        _ = scriptHelper(reloadAS, "Reload YogaSMCNC")
    }

    @IBOutlet weak var vMenubarIcon: NSTextField!
    @IBAction func setMenubarIcon(_ sender: NSTextField) {
        if vMenubarIcon.stringValue == "" {
            guard defaults.value(forKey: "Title") != nil else { return }
            defaults.removeObject(forKey: "Title")
        } else {
            if defaults.string(forKey: "Title") == vMenubarIcon.stringValue { return }
            defaults.setValue(vMenubarIcon.stringValue, forKey: "Title")
        }
        _ = scriptHelper(reloadAS, "Reload YogaSMCNC")
    }

    @IBOutlet weak var vHideCapsLock: NSButton!
    @IBAction func toggleHideCapsLock(_ sender: NSButton) {
        defaults.setValue(vHideCapsLock.state == .on, forKey: "HideCapsLock")
        _ = scriptHelper(reloadAS, "Reload YogaSMCNC")
    }

    @IBAction func vClearEvents(_ sender: NSButton) {
        _ = scriptHelper(stopAS, "Stop YogaSMCNC")
        defaults.removeObject(forKey: "Events")
        _ = scriptHelper(startAS, "Start YogaSMCNC")
    }

    // Idea
    @IBOutlet weak var vFnKeyRadio: NSButton!
    @IBOutlet weak var vFxKeyRadio: NSButton!

    @IBOutlet weak var vBatteryID: NSTextField!
    @IBOutlet weak var vBatteryTemperature: NSTextField!
    @IBOutlet weak var vCycleCount: NSTextField!
    @IBOutlet weak var vMfgDate: NSTextField!

    @IBOutlet weak var vConservationMode: NSButton!
    @IBOutlet weak var vRapidChargeMode: NSButton!

    @IBOutlet weak var vCamera: NSTextField!
    @IBOutlet weak var vBluetooth: NSTextField!
    @IBOutlet weak var vWireless: NSTextField!
    @IBOutlet weak var vWWAN: NSTextField!
    @IBOutlet weak var vGraphics: NSTextField!

    // Think

    @IBOutlet weak var vChargeThresholdStart: NSTextField!
    @IBOutlet weak var vChargeThresholdStop: NSTextField!

    @IBOutlet weak var vPowerLEDSlider: NSSlider!
    @IBOutlet weak var vStandbyLEDSlider: NSSlider!
    @IBOutlet weak var vThinkDotSlider: NSSliderCell!
    @IBOutlet weak var vCustomLEDSlider: NSSlider!
    @IBOutlet weak var vCustomLEDList: NSPopUpButton!

    @IBOutlet weak var vFanSpeed: NSTextField!
    @IBOutlet weak var vSecondFan: NSButton!
    @IBOutlet weak var vFanStop: NSButton!
    @IBOutlet weak var vDisableFan: NSButton!
    @IBOutlet weak var vSaveFanLevel: NSButton!
    @IBOutlet weak var vMuteLEDFixup: NSButton!

    // Main

    @IBOutlet weak var mainTabView: NSTabView!
    @IBOutlet weak var ideaViewItem: NSTabViewItem!
    @IBOutlet weak var thinkViewItem: NSTabViewItem!

    @IBOutlet weak var vDYTCRevision: NSTextField!
    @IBOutlet weak var vDYTCFuncMode: NSTextField!
    @IBOutlet weak var DYTCSlider: NSSlider!
    @IBAction func DYTCset(_ sender: NSSlider) {
        _ = sendString("DYTCMode", DYTCCommand[DYTCSlider.integerValue], service)
        if let dict = getDictionary("DYTC", service) {
            updateDYTC(dict)
        }
    }

    @IBOutlet weak var backlightSlider: NSSlider!
    @IBAction func backlightSet(_ sender: NSSlider) {
        if !sendNumber("BacklightLevel", backlightSlider.integerValue, service) {
            let backlightLevel = getNumber("BacklightLevel", service)
            if backlightLevel != -1 {
                backlightSlider.integerValue = backlightLevel
            } else {
                backlightSlider.isEnabled = false
            }
        }
    }

    @IBOutlet weak var autoSleepCheck: NSButton!
    @IBOutlet weak var yogaModeCheck: NSButton!
    @IBOutlet weak var indicatorCheck: NSButton!
    @IBOutlet weak var muteCheck: NSButton!
    @IBOutlet weak var micMuteCheck: NSButton!
    @IBAction func autoBacklightSet(_ sender: NSButton) {
        let val = ((autoSleepCheck.state == .on) ? 1 << 0 : 0) +
                ((yogaModeCheck.state == .on) ? 1 << 1 : 0) +
                ((indicatorCheck.state == .on) ? 1 << 2 : 0) +
                ((muteCheck.state == .on) ? 1 << 3 : 0) +
                ((micMuteCheck.state == .on) ? 1 << 4 : 0)
        if !sendNumber("AutoBacklight", val, service) {
            let autoBacklight = getNumber("AutoBacklight", service)
            if autoBacklight != -1 {
                autoSleepCheck.state = ((autoBacklight & (1 << 0)) != 0) ? .on : .off
                yogaModeCheck.state =  ((autoBacklight & (1 << 1)) != 0) ? .on : .off
                indicatorCheck.state =  ((autoBacklight & (1 << 2)) != 0) ? .on : .off
                muteCheck.state =  ((autoBacklight & (1 << 3)) != 0) ? .on : .off
                micMuteCheck.state =  ((autoBacklight & (1 << 4)) != 0) ? .on : .off
            } else {
                autoSleepCheck.isEnabled = false
                yogaModeCheck.isEnabled = false
                indicatorCheck.isEnabled = false
                muteCheck.isEnabled = false
                micMuteCheck.isEnabled = false
            }
        }
    }

    @IBOutlet weak var vClamshellMode: NSButton!
    @IBAction func vClamshellModeSet(_ sender: NSButton) {
        _ = sendBoolean("ClamshellMode", (vClamshellMode.state == .on), service)
    }

    override func mainViewDidLoad() {
        super.mainViewDidLoad()
        os_log(#function, type: .info)
        // nothing
    }

    func updateDYTC(_ dict: NSDictionary) {
        if let ver = dict["Revision"] as? NSNumber,
           let subver = dict["SubRevision"] as? NSNumber {
            vDYTCRevision.stringValue = "\(ver.intValue).\(subver.intValue)"
        } else {
            vDYTCRevision.stringValue = "Unknown"
        }
        if let funcMode = dict["FuncMode"] as? String {
            vDYTCFuncMode.stringValue = funcMode
        } else {
            vDYTCFuncMode.stringValue = "Unknown"
        }
        if let perfMode = dict["PerfMode"] as? String {
            if perfMode == "Quiet" {
                DYTCSlider.integerValue = 0
            } else if perfMode == "Balance" {
                DYTCSlider.integerValue = 1
            } else if perfMode == "Performance" {
                DYTCSlider.integerValue = 2
            } else if perfMode == "Performance (Reduced as lapmode active)" {
                DYTCSlider.integerValue = 2
            } else {
                DYTCSlider.isEnabled = false
            }
        } else {
            DYTCSlider.isEnabled = false
        }
    }

    func updateMain(_ props: NSDictionary) {
        if let val = props["AutoBacklight"] as? NSNumber {
            let autoBacklight = val.intValue
            autoSleepCheck.state = ((autoBacklight & (1 << 0)) != 0) ? .on : .off
            yogaModeCheck.state =  ((autoBacklight & (1 << 1)) != 0) ? .on : .off
            indicatorCheck.state =  ((autoBacklight & (1 << 2)) != 0) ? .on : .off
            muteCheck.state =  ((autoBacklight & (1 << 3)) != 0) ? .on : .off
            micMuteCheck.state =  ((autoBacklight & (1 << 4)) != 0) ? .on : .off
        } else {
            autoSleepCheck.isEnabled = false
            yogaModeCheck.isEnabled = false
            indicatorCheck.isEnabled = false
            micMuteCheck.isEnabled = false
        }
        #if !DEBUG
        muteCheck.isEnabled = false
        if muteCheck.state == .on {
            muteCheck.state = .off
            autoBacklightSet(muteCheck)
        }
        #endif

        if let val = props["BacklightLevel"] as? NSNumber {
            backlightSlider.integerValue = val.intValue
        } else {
            backlightSlider.isEnabled = false
        }

        if let dict = props["DYTC"]  as? NSDictionary {
            updateDYTC(dict)
        } else {
            vDYTCRevision.stringValue = "Unsupported"
            vDYTCFuncMode.isHidden = true
            DYTCSlider.isHidden = true
        }

        if defaults.bool(forKey: "HideIcon") {
            vHideMenubarIcon.state = .on
        } else {
            vHideMenubarIcon.state = .off
            vMenubarIcon.isEnabled = true
        }
        vMenubarIcon.stringValue = defaults.string(forKey: "Title") ?? ""

        vHideCapsLock.state = defaults.bool(forKey: "HideCapsLock") ? .on : .off

        if let val = props["ClamshellMode"] as? Bool {
            vClamshellMode.isEnabled = true
            vClamshellMode.state = val ? .on : .off
        }
    }

    override func willSelect() {
        guard service != 0, sendBoolean("Update", true, service) else { return }

        guard let props = getProperties(service) else {
            os_log("Unable to acquire driver properties!", type: .fault)
            return
        }

        if let val = props["VersionInfo"] as? NSString {
            vVersion.stringValue = val as String
        } else {
            os_log("Unable to identify driver version!", type: .fault)
            return
        }

        if let val = props["EC Capability"] as? NSString {
            vECRead.stringValue = val as String
        } else {
            os_log("Unable to identify EC capability!", type: .fault)
            return
        }

        updateMain(props)

        switch props["IOClass"] as? NSString {
        case "IdeaVPC":
            vClass.stringValue = "Idea"
            updateIdea(props)
            #if !DEBUG
            mainTabView.removeTabViewItem(thinkViewItem)
            #endif
        case "ThinkVPC":
            vClass.stringValue = "Think"
            updateThink(props)
            #if !DEBUG
            mainTabView.removeTabViewItem(ideaViewItem)
            #endif
        case "YogaHIDD":
            vClass.stringValue = "HIDD"
            mainTabView.removeTabViewItem(ideaViewItem)
            mainTabView.removeTabViewItem(thinkViewItem)
        default:
            vClass.stringValue = "Unsupported"
            mainTabView.removeTabViewItem(ideaViewItem)
            mainTabView.removeTabViewItem(thinkViewItem)
        }
    }
}
