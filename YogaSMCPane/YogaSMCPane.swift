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


func getBoolean(_ key: String, _ io_service: io_service_t) -> Bool {
    if let rvalue = IORegistryEntryCreateCFProperty(io_service, key as CFString, kCFAllocatorDefault, 0) {
        return rvalue.takeRetainedValue() as! Bool
    }
    return false
}

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

func sendBoolean(_ key: String, _ value: Bool, _ io_service: io_service_t) -> kern_return_t {
    return IORegistryEntrySetCFProperty(io_service, key as CFString, value as CFBoolean)
}

func sendNumber(_ key: String, _ value: Int, _ io_service: io_service_t) -> kern_return_t {
    return IORegistryEntrySetCFProperty(io_service, key as CFString, value as CFNumber)
}

func sendString(_ key: String, _ value: String, _ io_service: io_service_t) -> kern_return_t {
    return IORegistryEntrySetCFProperty(io_service, key as CFString, value as CFString)
}

@available(OSX 10.15, *)
class YogaSMCPane : NSPreferencePane {
    var io_service : io_service_t = 0

    var unsupported = false

    @IBOutlet weak var vVersion: NSTextField!
    @IBOutlet weak var vBuild: NSTextField!
    @IBOutlet weak var vClass: NSTextField!
    @IBOutlet weak var vECRead: NSTextField!

    // Idea
    @IBOutlet weak var vFnKeyCheck: NSButton!
    @IBAction func FnKeySet(_ sender: NSButton) {
        if kIOReturnSuccess != sendBoolean("FnlockMode", vFnKeyCheck.state == .on, io_service) {
            vFnKeyCheck.state = getBoolean("FnlockMode", io_service) ? .on : .off
        }
    }
    @IBOutlet weak var vCamera: NSTextField!
    @IBOutlet weak var vBluetooth: NSTextField!
    @IBOutlet weak var vWireless: NSTextField!
    @IBOutlet weak var vWWAN: NSTextField!
    @IBOutlet weak var vGraphics: NSTextField!
    
    @IBOutlet weak var TabView: NSTabView!
    @IBOutlet weak var IdeaViewItem: NSTabViewItem!
    @IBOutlet weak var ThinkViewItem: NSTabViewItem!
    
    @IBOutlet weak var backlightSlider: NSSlider!
    @IBAction func backlightSet(_ sender: NSSlider) {
        if kIOReturnSuccess != sendNumber("BacklightLevel", backlightSlider.integerValue, io_service) {
            updateBacklight()
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
                ((micMuteCheck.state == .on) ? 1 << 3 : 0)
        if (sendNumber("AutoBacklight", val, io_service) != kIOReturnSuccess) {
            updateAutoBacklight()
        }
    }

    override func mainViewDidLoad() {
//        UserDefaults.standard.set("value", forKey: "testKey")
//        let rvalue = UserDefaults.standard.string(forKey: "testKey")

        super.mainViewDidLoad()
        // nothing
    }

    func updateIdea() {
        if let rPrimeKey = getString("PrimeKeyType", io_service) {
            vFnKeyCheck.title = rPrimeKey
            vFnKeyCheck.state = getBoolean("FnlockMode", io_service) ? .on : .off
        } else {
            vFnKeyCheck.isHidden = true
        }

        if let rvalue = IORegistryEntryCreateCFProperty(io_service, "Capability" as CFString, kCFAllocatorDefault, 0) {
            let dict = rvalue.takeRetainedValue() as! NSDictionary
            vCamera.stringValue = dict.value(forKey: "Camera") as! Bool ? "Yes" : "No"
            vBluetooth.stringValue = dict.value(forKey: "Bluetooth") as! Bool ? "Yes" : "No"
            vWireless.stringValue = dict.value(forKey: "Wireless") as! Bool ? "Yes" : "No"
            vWWAN.stringValue = dict.value(forKey: "3G") as! Bool ? "Yes" : "No"
            vGraphics.stringValue = dict.value(forKey: "Graphics") as! NSString as String
        }
        return
    }

    func updateThink() {
        return
    }

    func updateBacklight() {
        let backlightLevel = getNumber("BacklightLevel", io_service)
        if (backlightLevel != -1) {
            backlightSlider.integerValue = backlightLevel
        } else {
            backlightSlider.isEnabled = false
        }
    }

    func updateAutoBacklight() {
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
    }
    
    func update() {
        updateAutoBacklight()
        updateBacklight()

        if vClass.stringValue == "Idea" {
            updateIdea()
        } else if vClass.stringValue == "Think" {
            updateThink()
        }
    }

    override func awakeFromNib() {
        io_service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("YogaVPC"))

        if (io_service == 0) {
            return
        }

        switch IOObjectCopyClass(io_service).takeRetainedValue() as NSString {
        case "IdeaVPC":
            vClass.stringValue = "Idea"
            TabView.removeTabViewItem(ThinkViewItem)
            indicatorCheck.isHidden = true
            muteCheck.isHidden = true
            micMuteCheck.isHidden = true
        case "ThinkVPC":
            vClass.stringValue = "Think"
            TabView.removeTabViewItem(IdeaViewItem)
        case "YogaVPC":
            vClass.stringValue = "Generic"
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

        if let rECCap = getString("EC Capability", io_service) {
            vECRead.stringValue = rECCap
        } else {
            vECRead.stringValue = "Unavailable"
        }

        update()
    }
}
