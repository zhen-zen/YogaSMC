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

let DYTCCommand = ["L", "M", "H"]
let thinkLEDCommand = [0, 0x80, 0xA0, 0xC0]
let thinkBatteryName = ["BAT_ANY", "BAT_PRIMARY", "BAT_SECONDARY"]

// from https://ffried.codes/2018/01/20/the-internals-of-the-macos-hud/
@objc enum OSDImage: CLongLong {
    case kBrightness = 1
    case brightness2 = 2
    case kVolume = 3
    case kMute = 4
    case volume5 = 5
    case kEject = 6
    case brightness7 = 7
    case brightness8 = 8
    case kAirportRange = 9
    case wireless2Forbid = 10
    case kBright = 11
    case kBrightOff = 12
    case kBright13 = 13
    case kBrightOff14 = 14
    case Ajar = 15
    case mute16 = 16
    case volume17 = 17
    case empty18 = 18
    case kRemoteLinkedGeneric = 19
    case kRemoteSleepGeneric = 20 // will put into sleep
    case muteForbid = 21
    case volumeForbid = 22
    case volume23 = 23
    case empty24 = 24
    case kBright25 = 25
    case kBrightOff26 = 26
    case backlightonForbid = 27
    case backlightoffForbid = 28
    /* and more cases from 1 to 28 (except 18 and 24) */
}

// from https://github.com/w0lfschild/macOS_headers/blob/master/macOS/CoreServices/OSDUIHelper/1/OSDUIHelperProtocol-Protocol.h

@objc protocol OSDUIHelperProtocol {

    @objc func showFullScreenImage(_ img: OSDImage, onDisplayID: CGDirectDisplayID, priority: CUnsignedInt, msecToAnimate: CUnsignedInt)

    @objc func fadeClassicImageOnDisplay(_ img: CUnsignedInt)

    @objc func showImageAtPath(_ img: NSString, onDisplayID: CGDirectDisplayID, priority: CUnsignedInt, msecUntilFade: CUnsignedInt, withText: NSString)

    @objc func showImage(_ img: OSDImage, onDisplayID: CGDirectDisplayID, priority: CUnsignedInt, msecUntilFade: CUnsignedInt, filledChiclets: CUnsignedInt, totalChiclets: CUnsignedInt, locked: CBool)

    @objc func showImage(_ img: OSDImage, onDisplayID: CGDirectDisplayID, priority: CUnsignedInt, msecUntilFade: CUnsignedInt, withText: NSString)

    @objc func showImage(_ img: OSDImage, onDisplayID: CGDirectDisplayID, priority: CUnsignedInt, msecUntilFade: CUnsignedInt)
}

func getBoolean(_ key: String, _ io_service: io_service_t) -> Bool {
    guard let rvalue = IORegistryEntryCreateCFProperty(io_service, key as CFString, kCFAllocatorDefault, 0) else {
        return false
    }

    return rvalue.takeRetainedValue() as! Bool
}

func getNumber(_ key: String, _ io_service: io_service_t) -> Int {
    guard let rvalue = IORegistryEntryCreateCFProperty(io_service, key as CFString, kCFAllocatorDefault, 0) else {
        return -1
    }

    return rvalue.takeRetainedValue() as! Int
}

func getString(_ key: String, _ io_service: io_service_t) -> String? {
    guard let rvalue = IORegistryEntryCreateCFProperty(io_service, key as CFString, kCFAllocatorDefault, 0) else {
        return nil
    }

    return rvalue.takeRetainedValue() as! NSString as String
}

func getDictionary(_ key: String, _ io_service: io_service_t) -> NSDictionary? {
    guard let rvalue = IORegistryEntryCreateCFProperty(io_service, key as CFString, kCFAllocatorDefault, 0) else {
        return nil
    }

    return rvalue.takeRetainedValue() as? NSDictionary
}

func sendBoolean(_ key: String, _ value: Bool, _ io_service: io_service_t) -> Bool {
    return (kIOReturnSuccess == IORegistryEntrySetCFProperty(io_service, key as CFString, value as CFBoolean))
}

func sendNumber(_ key: String, _ value: Int, _ io_service: io_service_t) -> Bool {
    return (kIOReturnSuccess == IORegistryEntrySetCFProperty(io_service, key as CFString, value as CFNumber))
}

func sendString(_ key: String, _ value: String, _ io_service: io_service_t) -> Bool {
    return (kIOReturnSuccess == IORegistryEntrySetCFProperty(io_service, key as CFString, value as CFString))
}

class YogaSMCPane : NSPreferencePane {
    var io_service : io_service_t = 0
    var thinkBatteryNumber = 0

    @IBOutlet weak var vVersion: NSTextField!
    @IBOutlet weak var vBuild: NSTextField!
    @IBOutlet weak var vClass: NSTextField!
    @IBOutlet weak var vECRead: NSTextField!

    // Idea
    @IBOutlet weak var FunctionKey: NSStackView!
    @IBOutlet weak var vFnKeyRadio: NSButton!
    @IBOutlet weak var vFxKeyRadio: NSButton!
    @IBAction func vFnKeySet(_ sender: NSButton) {
        if !sendBoolean("FnlockMode", vFnKeyRadio.state == .on, io_service) {
            vFnKeyRadio.state = getBoolean("FnlockMode", io_service) ? .on : .off
        }
    }

    @IBOutlet weak var vConservationMode: NSButton!
    @IBOutlet weak var vRapidChargeMode: NSButton!
    @IBOutlet weak var vBatteryID: NSTextField!
    @IBOutlet weak var vBatteryTemperature: NSTextField!
    @IBOutlet weak var vCycleCount: NSTextField!
    @IBOutlet weak var vMfgDate: NSTextField!

    @IBAction func vConservationModeSet(_ sender: NSButton) {
        if !sendBoolean("ConservationMode", vConservationMode.state == .on, io_service) {
            vConservationMode.state = getBoolean("ConservationMode", io_service) ? .on : .off
        }
    }
    
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
                let old = (vStart as! Int32) & 0xff
                if (old < 0 || old > 99) {
                    vChargeThresholdStart.isEnabled = false
                    return
                } else if old == vChargeThresholdStart.integerValue {
                    return
                }
                #if DEBUG
                let prompt = String(format:"Start %d", vChargeThresholdStart.integerValue)
                OSD(prompt)
                #endif
                _ = sendNumber("setCMstart", vChargeThresholdStart.integerValue, io_service)
            }
        }
    }

    @IBAction func vChargeThresholdStopSet(_ sender: NSTextField) {
        if let dict = getDictionary(thinkBatteryName[thinkBatteryNumber], io_service) {
            if let vStop = dict["BCSG"] as? NSNumber {
                let old = (vStop as! Int32) & 0xff
                if (old < 1 || old > 100) {
                    vChargeThresholdStop.isEnabled = false
                    return
                } else if old == vChargeThresholdStop.integerValue {
                    return
                }
                #if DEBUG
                let prompt = String(format:"Stop %d", vChargeThresholdStop.integerValue)
                OSD(prompt)
                #endif
                _ = sendNumber("setCMstop", vChargeThresholdStop.integerValue == 100 ? 0 : vChargeThresholdStop.integerValue, io_service)
            }
        }
    }

    @IBOutlet weak var vPowerLEDSlider: NSSlider!
    @IBOutlet weak var vStandbyLEDSlider: NSSlider!
    @IBOutlet weak var vThinkDotSlider: NSSliderCell!
    @IBOutlet weak var vCustomLEDSlider: NSSlider!
    @IBOutlet weak var vCustomLEDList: NSPopUpButton!
    @IBAction func vPowerLEDSet(_ sender: NSSlider) {
        if (!sendNumber("LED", thinkLEDCommand[vPowerLEDSlider.integerValue] + 0x00, io_service)) {
            return
        }
    }

    @IBAction func vStandbyLEDSet(_ sender: NSSlider) {
        if (!sendNumber("LED", thinkLEDCommand[vStandbyLEDSlider.integerValue] + 0x07, io_service)) {
            return
        }
    }

    @IBAction func vThinkDotSet(_ sender: NSSlider) {
        if (!sendNumber("LED", thinkLEDCommand[vThinkDotSlider.integerValue] + 0x0A, io_service)) {
            return
        }
    }

    @IBAction func vCustomLEDSet(_ sender: NSSlider) {
        let value = thinkLEDCommand[vCustomLEDSlider.integerValue] + vCustomLEDList.indexOfSelectedItem
        #if DEBUG
        let prompt = String(format:"LED 0x%02X", value)
        OSD(prompt)
        #endif
        if (!sendNumber("LED", value, io_service)) {
            return
        }
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
            if (backlightLevel != -1) {
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
                ((micMuteCheck.state == .on) ? 1 << 3 : 0)
        if (!sendNumber("AutoBacklight", val, io_service)) {
            let autoBacklight = getNumber("AutoBacklight", io_service)
            if (autoBacklight != -1) {
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
//        UserDefaults.standard.set("value", forKey: "testKey")
//        let rvalue = UserDefaults.standard.string(forKey: "testKey")

        super.mainViewDidLoad()
        // nothing
    }

    func updateDYTC(_ dict: NSDictionary) {
        if let ver = dict["Revision"] as? NSNumber,
           let subver = dict["SubRevision"] as? NSNumber {
            vDYTCRevision.stringValue = "\(ver as! Int).\(subver as! Int)"
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
            if let ID = dict["ID"] as? String {
                vBatteryID.stringValue = ID
            } else {
                vBatteryID.stringValue = "Unknown"
            }
            if let count = dict["Cycle count"] as? String {
                vCycleCount.stringValue = count
            } else {
                vCycleCount.stringValue = "Unknown"
            }
            if let temp = dict["Temperature"] as? String {
                vBatteryTemperature.stringValue = temp
            } else {
                vBatteryTemperature.stringValue = "Unknown"
            }
            if let mfgDate = dict["Manufacture date"] as? String {
                vMfgDate.stringValue = mfgDate
            } else {
                vMfgDate.stringValue = "Unknown"
            }
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

    func OSD(_ prompt: String) {
        // from https://ffried.codes/2018/01/20/the-internals-of-the-macos-hud/
        let conn = NSXPCConnection(machServiceName: "com.apple.OSDUIHelper", options: [])
        conn.remoteObjectInterface = NSXPCInterface(with: OSDUIHelperProtocol.self)
        conn.interruptionHandler = { print("Interrupted!") }
        conn.invalidationHandler = { print("Invalidated!") }
        conn.resume()

        let target = conn.remoteObjectProxyWithErrorHandler { print("Failed: \($0)") }
        guard let helper = target as? OSDUIHelperProtocol else { return } //fatalError("Wrong type: \(target)") }

        helper.showImageAtPath("/System/Library/CoreServices/OSDUIHelper.app/Contents/Resources/kBright.pdf", onDisplayID: CGMainDisplayID(), priority: 0x1f4, msecUntilFade: 2000, withText: prompt as NSString)
    }

    func updateThinkBattery() -> Bool {
        _ = sendNumber("Battery", thinkBatteryNumber, io_service)
        if let dict = getDictionary(thinkBatteryName[thinkBatteryNumber], io_service) {
            if let vStart = dict["BCTG"] as? NSNumber,
               let vStop = dict["BCSG"] as? NSNumber {
                let rStart = vStart as! Int32
                let rStop = vStop as! Int32
                if rStart >= 0,
                   rStop >= 0 {
                    vChargeThresholdStart.isEnabled = true
                    vChargeThresholdStop.isEnabled = true
                    vChargeThresholdStart.intValue = rStart & 0xff
                    vChargeThresholdStop.intValue = rStop & 0xff
                    return true
                }
            }
        }
        thinkBatteryNumber += 1
        return false
    }
    
    func awakeThink(_ props: NSDictionary) {
        while thinkBatteryNumber <= 2 {
            if updateThinkBattery() {
                break
            }
        }
    }

    override func awakeFromNib() {
        io_service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("YogaVPC"))

        if (io_service == 0 || !sendBoolean("Update", true, io_service)) {
            return
        }

        var CFProps : Unmanaged<CFMutableDictionary>? = nil
        guard (kIOReturnSuccess == IORegistryEntryCreateCFProperties(io_service, &CFProps, kCFAllocatorDefault, 0) && CFProps != nil) else {
            return
        }

        guard let props = CFProps?.takeRetainedValue() as NSDictionary? else {
            return
        }

        if let val = props["YogaSMC,Build"] as? NSString {
            vBuild.stringValue = val as String
        } else {
            vBuild.stringValue = "Unknown"
            return
        }

        if let val = props["YogaSMC,Version"] as? NSString {
            vVersion.stringValue = val as String
        } else {
            vVersion.stringValue = "Unknown"
            return
        }

        if let val = props["EC Capability"] as? NSString {
            vECRead.stringValue = val as String
        } else {
            vECRead.stringValue = "Unknown"
            return
        }

        if let val = props["AutoBacklight"] as? NSNumber {
            let autoBacklight = val as! Int
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

        if let val = props["BacklightLevel"] as? NSNumber {
            backlightSlider.integerValue = val as! Int
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
        case "YogaVPC":
            vClass.stringValue = "Generic"
            TabView.removeTabViewItem(IdeaViewItem)
            TabView.removeTabViewItem(ThinkViewItem)
        default:
            vClass.stringValue = "Unsupported"
            TabView.removeTabViewItem(IdeaViewItem)
            TabView.removeTabViewItem(ThinkViewItem)
        }
    }
}
