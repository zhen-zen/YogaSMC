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
    let io_service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("YogaVPC"))
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
            if defaults.value(forKey: "Title") == nil {
                return
            }
            defaults.removeObject(forKey: "Title")
        } else {
            if defaults.string(forKey: "Title") == vMenubarIcon.stringValue {
                return
            }
            defaults.setValue(vMenubarIcon.stringValue, forKey: "Title")
        }
        _ = scriptHelper(reloadAS, "Reload YogaSMCNC")
    }

    // Idea
    @IBOutlet weak var FunctionKey: NSStackView!
    @IBOutlet weak var vFnKeyRadio: NSButton!
    @IBOutlet weak var vFxKeyRadio: NSButton!
    @IBAction func vFnKeySet(_ sender: NSButton) {
        if !sendBoolean("FnlockMode", vFnKeyRadio.state == .on, io_service) {
            vFnKeyRadio.state = getBoolean("FnlockMode", io_service) ? .on : .off
        }
    }

    @IBOutlet weak var vBatteryID: NSTextField!
    @IBOutlet weak var vBatteryTemperature: NSTextField!
    @IBOutlet weak var vCycleCount: NSTextField!
    @IBOutlet weak var vMfgDate: NSTextField!

    @IBOutlet weak var vConservationMode: NSButton!
    @IBAction func vConservationModeSet(_ sender: NSButton) {
        if !sendBoolean("ConservationMode", vConservationMode.state == .on, io_service) {
            vConservationMode.state = getBoolean("ConservationMode", io_service) ? .on : .off
        }
    }

    @IBOutlet weak var vRapidChargeMode: NSButton!

    @IBOutlet weak var vCamera: NSTextField!
    @IBOutlet weak var vBluetooth: NSTextField!
    @IBOutlet weak var vWireless: NSTextField!
    @IBOutlet weak var vWWAN: NSTextField!
    @IBOutlet weak var vGraphics: NSTextField!

    // Think

    @IBOutlet weak var vChargeThresholdStart: NSTextField!
    @IBOutlet weak var vChargeThresholdStop: NSTextField!
    @IBAction func vChargeThresholdStartSet(_ sender: NSTextFieldCell) {
        if let dict = getDictionary(thinkBatteryName[thinkBatteryNumber], io_service) {
            if let vStart = dict["BCTG"] as? NSNumber {
                let old = (vStart.intValue) & 0xff
                if old < 0 || old > 99 {
                    vChargeThresholdStart.isEnabled = false
                    return
                }
                if old == vChargeThresholdStart.integerValue {
                    return
                }
                #if DEBUG
                showOSD(String(format: "Start %d", vChargeThresholdStart.integerValue))
                #endif
                _ = sendNumber("setCMstart", vChargeThresholdStart.integerValue, io_service)
            }
        }
    }

    @IBAction func vChargeThresholdStopSet(_ sender: NSTextField) {
        if let dict = getDictionary(thinkBatteryName[thinkBatteryNumber], io_service) {
            if let vStop = dict["BCSG"] as? NSNumber {
                var old = (vStop.intValue) & 0xff
                if old < 0 || old > 99 {
                    vChargeThresholdStop.isEnabled = false
                    return
                }
                if old == 0 {
                    old = 100
                }
                if old == vChargeThresholdStop.integerValue {
                    return
                }
                #if DEBUG
                showOSD(String(format: "Stop %d", vChargeThresholdStop.integerValue))
                #endif
                _ = sendNumber("setCMstop", vChargeThresholdStop.integerValue == 100 ? 0 : vChargeThresholdStop.integerValue, io_service)
            }
        }
    }

    @IBOutlet weak var vPowerLEDSlider: NSSlider!
    @IBAction func vPowerLEDSet(_ sender: NSSlider) {
        if !sendNumber("LED", thinkLEDCommand[vPowerLEDSlider.integerValue] + 0x00, io_service) {
            return
        }
    }

    @IBOutlet weak var vStandbyLEDSlider: NSSlider!
    @IBAction func vStandbyLEDSet(_ sender: NSSlider) {
        if !sendNumber("LED", thinkLEDCommand[vStandbyLEDSlider.integerValue] + 0x07, io_service) {
            return
        }
    }

    @IBOutlet weak var vThinkDotSlider: NSSliderCell!
    @IBAction func vThinkDotSet(_ sender: NSSlider) {
        if !sendNumber("LED", thinkLEDCommand[vThinkDotSlider.integerValue] + 0x0A, io_service) {
            return
        }
    }

    @IBOutlet weak var vCustomLEDSlider: NSSlider!
    @IBOutlet weak var vCustomLEDList: NSPopUpButton!
    @IBAction func vCustomLEDSet(_ sender: NSSlider) {
        let value = thinkLEDCommand[vCustomLEDSlider.integerValue] + vCustomLEDList.indexOfSelectedItem
        #if DEBUG
        showOSD(String(format: "LED 0x%02X", value))
        #endif
        if !sendNumber("LED", value, io_service) {
            return
        }
    }

    @IBOutlet weak var vFanSpeed: NSTextField!
    @IBOutlet weak var vSecondFan: NSButton!
    @IBAction func vSecondFanSet(_ sender: NSButton) {
        defaults.setValue((vSecondFan.state == .on), forKey: "SecondThinkFan")
        _ = scriptHelper(reloadAS, "Reload YogaSMCNC")
    }
    @IBOutlet weak var vFanStop: NSButton!
    @IBAction func vFanStopSet(_ sender: NSButton) {
        defaults.setValue((vFanStop.state == .on), forKey: "AllowFanStop")
        _ = scriptHelper(reloadAS, "Reload YogaSMCNC")
    }
    @IBOutlet weak var vDisableFan: NSButton!
    @IBAction func vDisableFanSet(_ sender: NSButton) {
        defaults.setValue((vDisableFan.state == .on), forKey: "DisableFan")
        _ = scriptHelper(reloadAS, "Reload YogaSMCNC")
    }
    
    // Main

    @IBOutlet weak var TabView: NSTabView!
    @IBOutlet weak var IdeaViewItem: NSTabViewItem!
    @IBOutlet weak var ThinkViewItem: NSTabViewItem!

    @IBOutlet weak var vDYTCRevision: NSTextField!
    @IBOutlet weak var vDYTCFuncMode: NSTextField!
    @IBOutlet weak var DYTCSlider: NSSlider!
    @IBAction func DYTCset(_ sender: NSSlider) {
        _ = sendString("DYTCMode", DYTCCommand[DYTCSlider.integerValue], io_service)
        if let dict = getDictionary("DYTC", io_service) {
            updateDYTC(dict)
        }
    }

    @IBOutlet weak var backlightSlider: NSSlider!
    @IBAction func backlightSet(_ sender: NSSlider) {
        if !sendNumber("BacklightLevel", backlightSlider.integerValue, io_service) {
            let backlightLevel = getNumber("BacklightLevel", io_service)
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
        if !sendNumber("AutoBacklight", val, io_service) {
            let autoBacklight = getNumber("AutoBacklight", io_service)
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

    func updateIdeaBattery() {
        _ = sendBoolean("Battery", true, io_service)
        if let dict = getDictionary("Battery 0", io_service) {
            vBatteryID.stringValue = dict["ID"] as? String ?? "Unknown"
            vCycleCount.stringValue = dict["Cycle count"] as? String ?? "Unknown"
            vBatteryTemperature.stringValue = dict["Temperature"] as? String ?? "Unknown"
            vMfgDate.stringValue = dict["Manufacture date"] as? String ?? "Unknown"
        }
    }

    func awakeIdea(_ props: NSDictionary) {
        FunctionKey.isHidden = false
        updateIdeaBattery()

        if let val = props["PrimeKeyType"] as? NSString {
            vFnKeyRadio.title = val as String
            if let val = props["FnlockMode"] as? Bool {
                vFnKeyRadio.state = val ? .on : .off
            }
        } else {
            vFnKeyRadio.title = "Unknown"
            vFnKeyRadio.isEnabled = false
            vFxKeyRadio.isEnabled = false
            vFxKeyRadio.state = .on
        }

        if let val = props["ConservationMode"] as? Bool {
            vConservationMode.state = val ? .on : .off
        } else {
            vConservationMode.isEnabled = false
        }

        if let val = props["RapidChargeMode"] as? Bool {
            vRapidChargeMode.state = val ? .on : .off
            #if DEBUG
            vRapidChargeMode.isEnabled = true
            #endif
        } else {
            vRapidChargeMode.isEnabled = false
        }

        if let dict = props["Capability"]  as? NSDictionary {
            if let val = dict["Camera"] as? Bool {
                vCamera.textColor = val ? NSColor.systemGreen : NSColor.systemRed
            }
            if let val = dict["Bluetooth"] as? Bool {
                vBluetooth.textColor = val ? NSColor.systemGreen : NSColor.systemRed
            }
            if let val = dict["Wireless"] as? Bool {
                vWireless.textColor = val ? NSColor.systemGreen : NSColor.systemRed
            }
            if let val = dict["3G"] as? Bool {
                vWWAN.textColor = val ? NSColor.systemGreen : NSColor.systemRed
            }
            if let val = dict["Graphics"] as? NSString {
                vGraphics.toolTip = val as String
                switch val {
                case "Intel":
                    vGraphics.textColor = NSColor(red: 0/0xff, green: 0x71/0xff, blue: 0xc5/0xff, alpha: 1)
                case "ATI":
                    vGraphics.textColor = NSColor(red: 0x97/0xff, green: 0x0a/0xff, blue: 0x1b/0xff, alpha: 1)
                case "Nvidia":
                    vGraphics.textColor = NSColor(red: 0x76/0xff, green: 0xb9/0xff, blue: 0/0xff, alpha: 1)
                case "Intel and ATI":
                    vGraphics.textColor = NSColor(red: 0x97/0xff, green: 0x0a/0xff, blue: 0x1b/0xff, alpha: 1)
                case "Intel and Nvidia":
                    vGraphics.textColor = NSColor(red: 0x76/0xff, green: 0xb9/0xff, blue: 0/0xff, alpha: 1)
                default: break
                }
            }
        }
    }

    func updateThinkBattery() -> Bool {
        _ = sendNumber("Battery", thinkBatteryNumber, io_service)
        if let dict = getDictionary(thinkBatteryName[thinkBatteryNumber], io_service),
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
        if kIOReturnSuccess == IOServiceOpen(io_service, mach_task_self_, 0, &connect),
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

    func awakeThink(_ props: NSDictionary) {
        while thinkBatteryNumber <= 2 {
            if updateThinkBattery() {
                break
            }
        }
        updateThinkFan()
        #if DEBUG
        if !getBoolean("Dual fan", io_service) {
            vSecondFan.isEnabled = true
            vSecondFan.state = defaults.bool(forKey: "SecondThinkFan") ? .on : .off
        }
        #endif
        vFanStop.state = defaults.bool(forKey: "AllowFanStop") ? .on : .off
        if getBoolean("LEDSupport", io_service) {
            vPowerLEDSlider.isEnabled = true
            vStandbyLEDSlider.isEnabled = true
            vThinkDotSlider.isEnabled = true
            vCustomLEDSlider.isEnabled = true
        }
        vDisableFan.state = defaults.bool(forKey: "DisableFan") ? .on : .off
    }

    override func awakeFromNib() {
        guard io_service != 0, sendBoolean("Update", true, io_service) else { return }

        var CFProps: Unmanaged<CFMutableDictionary>?
        guard kIOReturnSuccess == IORegistryEntryCreateCFProperties(io_service, &CFProps, kCFAllocatorDefault, 0),
              CFProps != nil,
              let props = CFProps?.takeRetainedValue() as NSDictionary? else {
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

        switch props["IOClass"] as? NSString {
        case "IdeaVPC":
            vClass.stringValue = "Idea"
            awakeIdea(props)
            #if !DEBUG
            TabView.removeTabViewItem(ThinkViewItem)
            #endif
        case "ThinkVPC":
            vClass.stringValue = "Think"
            awakeThink(props)
            #if !DEBUG
            TabView.removeTabViewItem(IdeaViewItem)
            #endif
        case "YogaHIDD":
            vClass.stringValue = "HIDD"
            TabView.removeTabViewItem(IdeaViewItem)
            TabView.removeTabViewItem(ThinkViewItem)
        case "YogaVPC":
            vClass.stringValue = "Generic"
            TabView.removeTabViewItem(IdeaViewItem)
            TabView.removeTabViewItem(ThinkViewItem)
        default:
            vClass.stringValue = "Unsupported"
            TabView.removeTabViewItem(IdeaViewItem)
            TabView.removeTabViewItem(ThinkViewItem)
        }

        if defaults.object(forKey: "HideIcon") != nil {
            vHideMenubarIcon.isEnabled = true
            if defaults.bool(forKey: "HideIcon") {
                vHideMenubarIcon.state = .on
            } else {
                vHideMenubarIcon.state = .off
                vMenubarIcon.isEnabled = true
            }
            vMenubarIcon.stringValue = defaults.string(forKey: "Title") ?? ""
        }
    }
}
