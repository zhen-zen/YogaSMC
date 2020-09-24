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

func sendBoolean(_ key: String, _ value: Bool, _ io_service: io_service_t) -> kern_return_t {
    return IORegistryEntrySetCFProperty(io_service, key as CFString, value as CFBoolean)
}

func sendNumber(_ key: String, _ value: Int, _ io_service: io_service_t) -> Bool {
    return (kIOReturnSuccess == IORegistryEntrySetCFProperty(io_service, key as CFString, value as CFNumber))
}

func sendString(_ key: String, _ value: String, _ io_service: io_service_t) -> kern_return_t {
    return IORegistryEntrySetCFProperty(io_service, key as CFString, value as CFString)
}

class YogaSMCPane : NSPreferencePane {
    var io_service : io_service_t = 0

    var unsupported = false

    @IBOutlet weak var vVersion: NSTextField!
    @IBOutlet weak var vBuild: NSTextField!
    @IBOutlet weak var vClass: NSTextField!
    @IBOutlet weak var vECRead: NSTextField!

    // Idea
    @IBOutlet weak var vFnKeyRadio: NSButton!
    @IBOutlet weak var vFxKeyRadio: NSButton!
    @IBAction func vFnKeySet(_ sender: NSButton) {
        if kIOReturnSuccess == sendBoolean("FnlockMode", vFnKeyRadio.state == .on, io_service) {
            return
        }
        vFnKeyRadio.state = getBoolean("FnlockMode", io_service) ? .on : .off
    }
    @IBOutlet weak var vConservationMode: NSButton!
    @IBOutlet weak var vRapidChargeMode: NSButton!
    @IBOutlet weak var vBatteryID: NSTextField!
    @IBOutlet weak var vBatteryTemperature: NSTextField!
    @IBOutlet weak var vCycleCount: NSTextField!
    @IBOutlet weak var vMfgDate: NSTextField!

    @IBAction func vConservationModeSet(_ sender: NSButton) {
        if kIOReturnSuccess == sendBoolean("ConservationMode", vConservationMode.state == .on, io_service) {
            return
        }
        vConservationMode.state = getBoolean("ConservationMode", io_service) ? .on : .off
    }
    
    @IBOutlet weak var vCamera: NSTextField!
    @IBOutlet weak var vBluetooth: NSTextField!
    @IBOutlet weak var vWireless: NSTextField!
    @IBOutlet weak var vWWAN: NSTextField!
    @IBOutlet weak var vGraphics: NSTextField!

    // Think

    @IBOutlet weak var vPowerLEDSlider: NSSlider!
    @IBOutlet weak var vStandbyLEDSlider: NSSlider!
    @IBOutlet weak var vThinkDotSlider: NSSliderCell!
    @IBOutlet weak var vCustomLEDSlider: NSSlider!
    @IBOutlet weak var vCustomLEDList: NSPopUpButton!
    @IBAction func vPowerLEDSet(_ sender: NSSlider) {
        if (!sendNumber("LED", vPowerLEDSlider.integerValue * 0x40 + 0x00, io_service)) {
            return
        }
    }
    @IBAction func vStandbyLEDSet(_ sender: NSSlider) {
        if (!sendNumber("LED", vStandbyLEDSlider.integerValue * 0x40 + 0x07, io_service)) {
            return
        }
    }
    @IBAction func vThinkDotSet(_ sender: NSSlider) {
        if (!sendNumber("LED", vThinkDotSlider.integerValue * 0x40 + 0x0A, io_service)) {
            return
        }
    }
    @IBAction func vCustomLEDSet(_ sender: NSSlider) {
        let value = vCustomLEDSlider.integerValue * 0x40 + vCustomLEDList.indexOfSelectedItem
        let hex = String(format:"0x%02X", value)
        let prompt = "LED \(hex)"
        OSD(prompt)
        if (!sendNumber("LED", value, io_service)) {
            return
        }
    }
    @IBOutlet weak var TabView: NSTabView!
    @IBOutlet weak var IdeaViewItem: NSTabViewItem!
    @IBOutlet weak var ThinkViewItem: NSTabViewItem!

    @IBOutlet weak var backlightSlider: NSSlider!
    @IBAction func backlightSet(_ sender: NSSlider) {
        if !sendNumber("BacklightLevel", backlightSlider.integerValue, io_service) {
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
        if (!sendNumber("AutoBacklight", val, io_service)) {
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
            vFnKeyRadio.title = rPrimeKey
            vFnKeyRadio.state = getBoolean("FnlockMode", io_service) ? .on : .off
        } else {
            vFnKeyRadio.title = "Unknown"
            vFnKeyRadio.isEnabled = false
            vFxKeyRadio.isEnabled = false
            vFxKeyRadio.state = .on
        }

        vConservationMode.state = getBoolean("ConservationMode", io_service) ? .on : .off

        vRapidChargeMode.state = getBoolean("RapidChargeMode", io_service) ? .on : .off

        if let rvalue = IORegistryEntryCreateCFProperty(io_service, "Battery 0" as CFString, kCFAllocatorDefault, 0) {
            if let dict = rvalue.takeRetainedValue() as? NSDictionary {
                if let ID = dict.value(forKey: "ID") as? String {
                    vBatteryID.stringValue = ID
                } else {
                    vBatteryID.stringValue = "Unknown"
                }
                if let count = dict.value(forKey: "Cycle count") as? String {
                    vCycleCount.stringValue = count
                } else {
                    vCycleCount.stringValue = "Unknown"
                }
                if let temp = dict.value(forKey: "Temperature") as? String {
                    vBatteryTemperature.stringValue = temp
                } else {
                    vBatteryTemperature.stringValue = "Unknown"
                }
                if let mfgDate = dict.value(forKey: "Manufacture date") as? String {
                    vBatteryTemperature.stringValue = mfgDate
                } else {
                    vBatteryTemperature.stringValue = "Unknown"
                }
        }
//            Manufacture date
        }

        if let rvalue = IORegistryEntryCreateCFProperty(io_service, "Capability" as CFString, kCFAllocatorDefault, 0) {
            if let dict = rvalue.takeRetainedValue() as? NSDictionary {
                vCamera.stringValue = dict.value(forKey: "Camera") as! Bool ? "Yes" : "No"
                vBluetooth.stringValue = dict.value(forKey: "Bluetooth") as! Bool ? "Yes" : "No"
                vWireless.stringValue = dict.value(forKey: "Wireless") as! Bool ? "Yes" : "No"
                vWWAN.stringValue = dict.value(forKey: "3G") as! Bool ? "Yes" : "No"
                vGraphics.stringValue = dict.value(forKey: "Graphics") as! NSString as String
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
//            TabView.removeTabViewItem(ThinkViewItem)
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

        guard let rbuild = getString("YogaSMC,Build", io_service)  else {
            vBuild.stringValue = "Unknown"
            return
        }

        vBuild.stringValue = rbuild

        guard let rversion = getString("YogaSMC,Version", io_service) else {
            vVersion.stringValue = "Unknown"
            return
        }

        vVersion.stringValue = rversion

        if let rECCap = getString("EC Capability", io_service) {
            vECRead.stringValue = rECCap
        } else {
            vECRead.stringValue = "Unavailable"
        }

        update()
    }
}
