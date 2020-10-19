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
let siriAS = """
                tell application "System Events" to tell the front menu bar of process "SystemUIServer"
                  tell (first menu bar item whose description is "Siri")
                     perform action "AXPress"
                  end tell
                end tell
             """

let getAudioMutedAS = "output muted of (get volume settings)"
let setAudioMuteAS = "set volume with output muted"
let setAudioUnmuteAS = "set volume without output muted"

let getMicVolumeAS = "input volume of (get volume settings)"
let setMicVolumeAS = "set volume input volume %d"

var volume : Int32 = 50

// based on https://medium.com/macoclock/everything-you-need-to-do-to-launch-an-applescript-from-appkit-on-macos-catalina-with-swift-1ba82537f7c3
func scriptHelper(_ source: String, _ name: String) -> NSAppleEventDescriptor? {
    if let scpt = NSAppleScript(source: source) {
        var error: NSDictionary?
        let ret = scpt.executeAndReturnError(&error)
        if error == nil {
            return ret
        }
    }
    os_log("%s: failed to execute script", type: .error)
    return nil
}

func micMuteHelper(_ io_service : io_service_t) {
    guard let current = scriptHelper(getMicVolumeAS, "MicMute") else {
        return
    }
    if current.int32Value != 0 {
        if scriptHelper(String(format: setMicVolumeAS, 0), "MicMute") != nil {
            volume = current.int32Value
            _ = sendNumber("MicMuteLED", 2, io_service)
            showOSDRes("Mute", .MicOff)
        }
    } else {
        if scriptHelper(String(format: setMicVolumeAS, volume), "MicMute") != nil {
            volume = current.int32Value
            _ = sendNumber("MicMuteLED", 0, io_service)
            showOSDRes("Unmute", .Mic)
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
        if scriptHelper(prefpaneAS, "Prefpane") == nil {
            let alert = NSAlert()
            alert.messageText = "Failed to open Preferences"
            alert.informativeText = "Please install YogaSMCPane"
            alert.alertStyle = .warning
            alert.addButton(withTitle: "OK")
            alert.runModal()
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

