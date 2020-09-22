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

// FIXME
func getBoolean(_ key: String, _ io_service: io_service_t) -> Bool {
    let rvalue = IORegistryEntryCreateCFProperty(io_service, key as CFString, kCFAllocatorDefault, 0)
    if rvalue == nil {
        return false
    }
    return rvalue?.takeRetainedValue() as! Bool
}

// FIXME
func getNumber(_ key: String, _ io_service: io_service_t) -> Int {
    if let rvalue = IORegistryEntryCreateCFProperty(io_service, key as CFString, kCFAllocatorDefault, 0) {
        return rvalue.takeRetainedValue() as! Int
    }
    return -1
}

func getString(_ key: String, _ io_service: io_service_t) -> String? {
    if let rvalue = IORegistryEntryCreateCFProperty(io_service, key as CFString, kCFAllocatorDefault, 0) {
        return rvalue.takeRetainedValue() as! NSString as String
    }
    return nil
}

class YogaSMCPane : NSPreferencePane {
    var io_service : io_service_t = 0

    var unsupported = false

    @IBOutlet weak var vVersion: NSTextField!
    @IBOutlet weak var vBuild: NSTextField!
    @IBOutlet weak var vClass: NSTextField!
    @IBOutlet weak var vECRead: NSTextField!
    
    @IBOutlet weak var TabView: NSTabView!
    @IBOutlet weak var IdeaViewItem: NSTabViewItem!
    @IBOutlet weak var ThinkViewItem: NSTabViewItem!
    
    @IBOutlet weak var backlightSlider: NSSlider!

    @IBOutlet weak var autoSleepCheck: NSButton!
    @IBOutlet weak var yogaModeCheck: NSButton!
    @IBOutlet weak var indicatorCheck: NSButton!
    @IBOutlet weak var muteCheck: NSButton!
    @IBOutlet weak var micMuteCheck: NSButton!

    override func mainViewDidLoad() {
//        UserDefaults.standard.set("value", forKey: "testKey")
//        let rvalue = UserDefaults.standard.string(forKey: "testKey")

        super.mainViewDidLoad()
        // nothing
    }

    func updateIdea() {
        return
    }

    func updateThink() {
        return
    }

    func update() {
        let autoBacklight = getNumber("AutoBacklight", io_service)
        if (autoBacklight != -1) {
            autoSleepCheck.state = ((autoBacklight & (1 << 0)) != 0) ? NSControl.StateValue.on : NSControl.StateValue.off
            yogaModeCheck.state =  ((autoBacklight & (1 << 1)) != 0) ? NSControl.StateValue.on : NSControl.StateValue.off
            indicatorCheck.state =  ((autoBacklight & (1 << 2)) != 0) ? NSControl.StateValue.on : NSControl.StateValue.off
            muteCheck.state =  ((autoBacklight & (1 << 3)) != 0) ? NSControl.StateValue.on : NSControl.StateValue.off
            micMuteCheck.state =  ((autoBacklight & (1 << 4)) != 0) ? NSControl.StateValue.on : NSControl.StateValue.off
        } else {
            autoSleepCheck.isEnabled = false
            yogaModeCheck.isEnabled = false
            indicatorCheck.isEnabled = false
            muteCheck.isEnabled = false
            micMuteCheck.isEnabled = false
        }

        let backlightLevel = getNumber("BacklightLevel", io_service)
        if (backlightLevel != -1) {
            backlightSlider.integerValue = backlightLevel
        } else {
            backlightSlider.isEnabled = false
        }

        if getBoolean("FnlockMode", io_service) {
            vECRead.stringValue = "Available"
        } else {
            vECRead.stringValue = "Unavailable"
        }

        if vClass.stringValue == "Idea" {
            updateIdea()
        } else if vClass.stringValue == "Think" {
            updateThink()
        }
    }

    override func awakeFromNib() {
        io_service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("YogaSMC"))

        if (io_service == 0) {
            return
        }

        switch IOObjectCopyClass(io_service).takeRetainedValue() as NSString {
        case "IdeaSMC":
            vClass.stringValue = "Idea"
            TabView.removeTabViewItem(ThinkViewItem)
            indicatorCheck.isHidden = true
            muteCheck.isHidden = true
            micMuteCheck.isHidden = true
        case "ThinkSMC":
            vClass.stringValue = "Think"
            TabView.removeTabViewItem(IdeaViewItem)
        case "YogaSMC":
            vClass.stringValue = "Unknown"
            TabView.removeTabViewItem(IdeaViewItem)
            TabView.removeTabViewItem(ThinkViewItem)
        default:
            vClass.stringValue = "Unsupported"
            unsupported = true
        }

        if let rbuild = getString("YogaSMC,Build", io_service) {
            vBuild.stringValue = rbuild
        } else {
            vBuild.stringValue = "Unknown"
            unsupported = true
        }

        if let rversion = getString("YogaSMC,Version", io_service) {
            vVersion.stringValue = rversion
        } else {
            vVersion.stringValue = "Unknown"
            unsupported = true
        }

        if unsupported {
            return
        }

        update()
    }
}
