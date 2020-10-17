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

let searchAS = "tell application \"System Events\" to keystroke space using {command down, option down}"
let spotlightAS = "tell application \"System Events\" to keystroke space using command down"

let getAudioMutedAS = "output muted of (get volume settings)"
let setAudioMuteAS = "set volume with output muted"
let setAudioUnmuteAS = "set volume without output muted"

let getMicVolumeAS = "input volume of (get volume settings)"
let setMicVolumeAS = "set volume input volume %d"

var volume : Int32 = 50

func scriptHelper(_ source: String, _ name: String) -> Bool {
    if let scpt = NSAppleScript(source: source) {
        var error: NSDictionary?
        _ = scpt.executeAndReturnError(&error)
        if error == nil {
            return true
        }
    }
    os_log("%s: failed to execute script", type: .error)
    return false
}

func micMuteHelper() {
    if let scpt = NSAppleScript(source: getMicVolumeAS) {
        var error: NSDictionary?
        var ret = scpt.executeAndReturnError(&error)
        if error != nil {
            os_log("MicMute: failed to execute script", type: .error)
            return
        }
        let current = ret.int32Value
        if current != 0 {
            showOSD("Mute")
            if let mute = NSAppleScript(source: String(format: setMicVolumeAS, 0)) {
                ret = mute.executeAndReturnError(&error)
                if error != nil {
                    os_log("MicMute: failed to execute script", type: .error)
                    return
                }
            }
            volume = current
        } else {
            showOSD("Unmute")
            if let unmute = NSAppleScript(source: String(format: setMicVolumeAS, volume)) {
                ret = unmute.executeAndReturnError(&error)
                if error != nil {
                    os_log("MicMute: failed to execute script", type: .error)
                    return
                }
            }
        }
    }
}

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

