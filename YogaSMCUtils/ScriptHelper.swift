//
//  ScriptHelper.swift
//  YogaSMC
//
//  Created by Zhen on 10/14/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import AppKit
import Foundation
import os.log

let prefpaneAS = """
                    tell application "System Preferences"
                        reveal pane "org.zhen.YogaSMCPane"
                        activate
                    end tell
                 """
let sleepAS = "tell application \"System Events\" to sleep"

func prefpaneHelper() {
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
}

