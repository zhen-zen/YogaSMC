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

    var thinkBatteryNumber = -1

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

    @IBOutlet weak var vAlwaysOnUSBMode: NSButton!
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
    @IBOutlet weak var vPrimaryChargeThresholdStart: NSTextField!
    @IBOutlet weak var vPrimaryChargeThresholdStop: NSTextField!
    @IBOutlet weak var vSecondaryChargeThresholdStart: NSTextField!
    @IBOutlet weak var vSecondaryChargeThresholdStop: NSTextField!

    @IBOutlet weak var vPowerLEDSlider: NSSlider!
    @IBOutlet weak var vStandbyLEDSlider: NSSlider!
    @IBOutlet weak var vThinkDotSlider: NSSliderCell!
    @IBOutlet weak var vCustomLEDSlider: NSSlider!
    @IBOutlet weak var vCustomLEDList: NSPopUpButton!

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

    @IBOutlet weak var backlightSlider: NSSlider!

    @IBOutlet weak var autoSleepCheck: NSButton!
    @IBOutlet weak var yogaModeCheck: NSButton!
    @IBOutlet weak var indicatorCheck: NSButton!
    @IBOutlet weak var muteCheck: NSButton!
    @IBOutlet weak var micMuteCheck: NSButton!

    @IBOutlet weak var vClamshellMode: NSButton!

    override func mainViewDidLoad() {
        super.mainViewDidLoad()
        if #available(macOS 10.12, *) {
            os_log(#function, type: .info)
        }
        // nothing
    }

    override func willSelect() {
        guard service != 0, sendBoolean("Update", true, service) else { return }

        guard let props = getProperties(service) else {
            if #available(macOS 10.12, *) {
                os_log("Unable to acquire driver properties!", type: .fault)
            }
            return
        }

        if let val = props["VersionInfo"] as? NSString {
            vVersion.stringValue = val as String
        } else {
            if #available(macOS 10.12, *) {
                os_log("Unable to identify driver version!", type: .fault)
            }
            return
        }

        if let val = props["EC Capability"] as? NSString {
            vECRead.stringValue = val as String
        } else {
            if #available(macOS 10.12, *) {
                os_log("Unable to identify EC capability!", type: .fault)
            }
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
