//
//  ScriptHelper.swift
//  YogaSMC
//
//  Created by Zhen on 10/14/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import AppKit
import Foundation
import ScriptingBridge
import os.log

let prefpaneAS = """
                    tell application "System Preferences"
                        reveal pane "org.zhen.YogaSMCPane"
                        activate
                    end tell
                 """
let sleepAS = "tell application \"System Events\" to sleep"

let getMicVolumeAS = "input volume of (get volume settings)"

func prefpaneHelper() {
    guard let application : SystemPreferencesApplication = SBApplication(bundleIdentifier: "com.apple.systempreferences") else {
        return
    }
    let panes = application.panes!().reduce([String: SystemPreferencesPane](), { (dictionary, object) -> [String: SystemPreferencesPane] in
        let pane = object as! SystemPreferencesPane
        var dictionary = dictionary
        if let name = pane.name {
            dictionary[name] = pane
        }
        return dictionary
    })
    guard panes.count != 0 else {
        #if DEBUG
        showOSD("Please grant access \n to Apple Event")
        #endif
        // from https://medium.com/macoclock/everything-you-need-to-do-to-launch-an-applescript-from-appkit-on-macos-catalina-with-swift-1ba82537f7c3
        if let script = NSAppleScript(source: prefpaneAS) {
            var error: NSDictionary?
            script.executeAndReturnError(&error)
            if error != nil {
                os_log("Failed to open prefpane", type: .error)
                let alert = NSAlert()
                alert.messageText = "Failed to open Preferences"
                alert.informativeText = "Please install YogaSMCPane"
                alert.alertStyle = .warning
                alert.addButton(withTitle: "OK")
                alert.runModal()
            }
        }
        return
    }
    guard let pane = panes["YogaSMCPane"] else {
        let alert = NSAlert()
        alert.messageText = "YogaSMCPane not installed"
        alert.alertStyle = .warning
        alert.addButton(withTitle: "OK")
        alert.runModal()
        return
    }
    _ = pane.reveal?()
    application.activate()
}

