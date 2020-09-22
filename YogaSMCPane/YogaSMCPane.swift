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
    let rkey:CFString = key as NSString
    guard let rvalue = IORegistryEntryCreateCFProperty(io_service, rkey, kCFAllocatorDefault, 0).takeRetainedValue() as? Bool else {
        return false
    }
    return rvalue
}

func getString(_ key: String, _ io_service: io_service_t) -> String? {
    let rkey:CFString = key as NSString
    guard let rvalue = IORegistryEntryCreateCFProperty(io_service, rkey, kCFAllocatorDefault, 0).takeRetainedValue() as? NSString else {
        return nil
    }
    return rvalue as String
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
    
    override func mainViewDidLoad() {
//        UserDefaults.standard.set("value", forKey: "testKey")
//        let rvalue = UserDefaults.standard.string(forKey: "testKey")

        super.mainViewDidLoad()
        // nothing
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
        case "ThinkSMC":
            vClass.stringValue = "Think"
            TabView.removeTabViewItem(IdeaViewItem)
        case "YogaSMC":
            vClass.stringValue = "Unknown"
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

//        if getBoolean("ReadEC", io_service) {
//            vECRead.stringValue = "Available"
//        } else {
//            vECRead.stringValue = "Unavailable"
//        }
    }
}
