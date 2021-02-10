//
//  NotificationHandler.swift
//  YogaSMCNC
//
//  Created by Zhen on 12/22/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import AppKit
import os.log

func registerNotification(_ conf: inout SharedConfig) -> Bool {
    // fix warning per https://forums.swift.org/t/swift-5-2-pointers-and-coretext/34862
    var portContext: CFMachPortContext = withUnsafeMutableBytes(of: &conf) { conf in
        CFMachPortContext(version: 0, info: conf.baseAddress, retain: nil, release: nil, copyDescription: nil)
    }

    guard let notificationPort = CFMachPortCreate(kCFAllocatorDefault, notificationCallback, &portContext, nil),
       let runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, notificationPort, 0) else {
        os_log("Failed to create mach port", type: .error)
        return false
    }

    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, .defaultMode)

    if kIOReturnSuccess != IOConnectSetNotificationPort(conf.connect, 0, CFMachPortGetPort(notificationPort), 0) {
        os_log("Failed to set notification port", type: .error)
        return false
    }
    return true
}

func notificationCallback(
    _ port: CFMachPort?,
    _ msg: UnsafeMutableRawPointer?,
    _ size: CFIndex,
    _ info: UnsafeMutableRawPointer?
) {
    guard var conf = info?.load(as: SharedConfig.self) else {
        os_log("Invalid config", type: .error)
        return
    }

    guard let notification = msg?.load(as: SMCNotificationMessage.self) else {
        os_log("Invalid notification", type: .error)
        return
    }

    if let events = conf.events[notification.event] {
        let mod = conf.modifier.subtracting(.capsLock)
        if !mod.isEmpty,
           let desc = events[notification.data | UInt32(mod.rawValue)] {
            eventActuator(desc, notification.data, &conf)
        } else if let desc = events[notification.data] {
            eventActuator(desc, notification.data, &conf)
        } else if let desc = events[0] {
            eventActuator(desc, notification.data, &conf)
        } else {
            let value = String(format: "0x%04x:%d", notification.event, notification.data)
            showOSDRaw("\(NSLocalizedString("EventVar", comment: "Event "))\(value)")
            os_log("Event %s default data not found", type: .error, value)
        }
    } else {
        let value = String(format: "0x%04x:%d", notification.event, notification.data)
        showOSDRaw("\(NSLocalizedString("EventVar", comment: "Event "))\(value)")
        conf.events[notification.event] = [0: EventDesc("Event \(value)", nil, opt: "Unknown")]
        #if DEBUG
        os_log("Event %s", type: .debug, value)
        #endif
    }
}

func eventActuator(_ desc: EventDesc, _ data: UInt32, _ conf: inout SharedConfig) {
    switch desc.action {
    case .nothing:
        #if DEBUG
        os_log("%s: Do nothing", type: .info, desc.name)
        #endif
        if !desc.display { return }
        if desc.name.hasPrefix("Event ") {
            let prompt = NSLocalizedString("EventVar", comment: "Event ") + desc.name.dropFirst("Event ".count)
            showOSDRaw(prompt, desc.image)
        } else {
            showOSD(desc.name, desc.image)
        }
    case .script:
        if let scpt = desc.option {
            _ = scriptHelper(scpt, desc.name, desc.display ? desc.image : nil)
        } else {
            os_log("%s: script not found", type: .error, desc.name)
        }
    case .launchapp:
        if let name = desc.option,
           NSWorkspace.shared.launchApplication(name) {
            if desc.display { showOSD(desc.name, desc.image) }
        } else {
            os_log("%s: app name not found", type: .error, desc.name)
        }
    case .launchbundle:
        guard let identifier = desc.option else {
            os_log("%s: bundle not found", type: .error)
            return
        }
        let collection = NSRunningApplication.runningApplications(withBundleIdentifier: identifier)
        if collection.isEmpty {
            if #available(OSX 10.15, *) {
                guard let url = NSWorkspace.shared.urlForApplication(withBundleIdentifier: identifier) else {
                    os_log("%s: app url not found", type: .error, desc.name)
                    return
                }
                NSWorkspace.shared.openApplication(at: url, configuration: .init(), completionHandler: nil)
            } else {
                guard NSWorkspace.shared.launchApplication(withBundleIdentifier: identifier,
                                                           options: .default,
                                                           additionalEventParamDescriptor: nil,
                                                           launchIdentifier: nil) else {
                    os_log("%s: app identifier not found", type: .error, desc.name)
                    return
                }
            }
        } else {
            var hide = false
            for app in collection {
                if !app.isHidden {
                    hide = true
                    break
                }
            }
            for app in collection {
                if hide {
                    app.hide()
                } else {
                    app.activate(options: .activateIgnoringOtherApps)
                }
            }
            if hide { return }
        }
        if desc.display { showOSD(desc.name, desc.image) }
    case .airplane:
        airplaneModeHelper(desc.name, desc.display)
    case .bluetooth:
        bluetoothHelper(desc.name, desc.display)
    case .bluetoothdiscoverable:
        bluetoothDiscoverableHelper(desc.name, desc.display)
    case .backlight:
        if !desc.display { return }
        switch data {
        case 0:
            showOSDRes("Backlight", "Off", .kBacklightOff)
        case 1:
            showOSDRes("Backlight", "Low", .kBacklightLow)
        case 2:
            showOSDRes("Backlight", "High", .kBacklightHigh)
        default:
            showOSDRes("Backlight", "\(data)", .kBacklightHigh)
        }
    case .fnlock:
        if !desc.display { return }
        switch data {
        case 0:
            showOSDRes("FnKey", "Disabled", .kFunctionKeyOff)
        case 1:
            showOSDRes("FnKey", "Enabled", .kFunctionKeyOn)
        default:
            // reserved for media keyboard
            showOSDRes("FnKey", "\(data)", .kFunctionKeyOn)
        }
    case .keyboard:
        if !desc.display { return }
        showOSDRes("Keyboard", (data != 0) ? "Enabled" : "Disabled", (data != 0) ? .kKeyboard : .kKeyboardOff)
    case .micmute:
        micMuteHelper(conf.service, desc.name)
    case .desktop:
        CoreDockSendNotification("com.apple.showdesktop.awake" as CFString, nil)
    case .expose:
        CoreDockSendNotification("com.apple.expose.front.awake" as CFString, nil)
    case .mission:
        CoreDockSendNotification("com.apple.expose.awake" as CFString, nil)
    case .launchpad:
        CoreDockSendNotification("com.apple.launchpad.toggle" as CFString, nil)
    case .prefpane:
        prefpaneHelper()
    case .sleep:
        if desc.display {
            showOSDRes(desc.name, .kSleep)
            sleep(1)
        }
        _ = scriptHelper(sleepAS, desc.name)
    case .search:
        _ = scriptHelper(searchAS, desc.name)
    case .siri:
        _ = scriptHelper(siriAS, desc.name)
    case .spotlight:
        _ = scriptHelper(spotlightAS, desc.name)
    case .thermal:
        showOSD(desc.name, desc.image)
        os_log("%s: thermal event", type: .info, desc.name)
    case .wireless:
        wirelessHelper(desc.name, desc.display)
    default:
        #if DEBUG
        os_log("%s: Not implmented", type: .info, desc.name)
        #endif
        if desc.display {
            showOSD(desc.name, desc.image)
        }
    }
}
