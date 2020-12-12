//
//  IdeaSMCPane.swift
//  YogaSMCPane
//
//  Created by Zhen on 12/9/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import AppKit
import Foundation

extension YogaSMCPane {
    @IBAction func vFnKeySet(_ sender: NSButton) {
        if !sendBoolean("FnlockMode", vFnKeyRadio.state == .on, service) {
            vFnKeyRadio.state = getBoolean("FnlockMode", service) ? .on : .off
        }
    }

    @IBAction func vConservationModeSet(_ sender: NSButton) {
        if !sendBoolean("ConservationMode", vConservationMode.state == .on, service) {
            vConservationMode.state = getBoolean("ConservationMode", service) ? .on : .off
        }
    }

    func updateIdeaBattery(_ dict: NSDictionary) {
        vBatteryID.stringValue = dict["ID"] as? String ?? "Unknown"
        vCycleCount.stringValue = dict["Cycle count"] as? String ?? "Unknown"
        vBatteryTemperature.stringValue = dict["Temperature"] as? String ?? "Unknown"
        vMfgDate.stringValue = dict["Manufacture date"] as? String ?? "Unknown"
    }

    func updateIdeaCap(_ dict: NSDictionary) {
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
            case "ATI", "Intel and ATI":
                vGraphics.textColor = NSColor(red: 0x97/0xff, green: 0x0a/0xff, blue: 0x1b/0xff, alpha: 1)
            case "Nvidia", "Intel and Nvidia":
                vGraphics.textColor = NSColor(red: 0x76/0xff, green: 0xb9/0xff, blue: 0/0xff, alpha: 1)
            default: break
            }
        }
    }

    func updateIdea(_ props: NSDictionary) {
        vFunctionKeyView.isHidden = false
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

        if let dict = props["Battery 0"] as? NSDictionary {
            updateIdeaBattery(dict)
        }

        if let dict = props["Capability"] as? NSDictionary {
            updateIdeaCap(dict)
        }
    }
}
