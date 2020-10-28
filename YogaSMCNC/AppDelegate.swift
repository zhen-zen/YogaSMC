//
//  AppDelegate.swift
//  YogaSMCNC
//
//  Created by Zhen on 10/11/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import AppKit
import ApplicationServices
import NotificationCenter
import os.log
import ServiceManagement

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate {
    let defaults = UserDefaults(suiteName: "org.zhen.YogaSMC")!
    var conf = sharedConfig()

    var statusItem: NSStatusItem?
    @IBOutlet weak var appMenu: NSMenu!
    var hide = false
    var isThink = false
    var text: NSTextField?

    func updateThinkFan() {
        guard conf.connect != 0 else {
            return
        }
        var addr : UInt64 = 0x84
        var outputSize = 2
        var output : [UInt8] = [0, 0]
        if kIOReturnSuccess == IOConnectCallMethod(conf.connect, UInt32(kYSMCUCReadEC), &addr, 1, nil, 0, nil, nil, &output, &outputSize),
           outputSize == 2 {
            let vFanSpeed = Int32(output[0]) | Int32(output[1]) << 8
            appMenu.items[5].title = "Fan: \(vFanSpeed) rpm"
        } else {
            os_log("Failed to access EC", type: .error)
        }
        #if DEBUG
        outputSize = 1
        var name = "HFSP" // 0x2f
        if kIOReturnSuccess == IOConnectCallMethod(conf.connect, UInt32(kYSMCUCReadECName), nil, 0, &name, 4, nil, nil, &output, &outputSize) {
            appMenu.items[6].title = "HFSP: \(output[0])"
        }
        name = "HFNI" // 0x83
        if kIOReturnSuccess == IOConnectCallMethod(conf.connect, UInt32(kYSMCUCReadECName), nil, 0, &name, 4, nil, nil, &output, &outputSize) {
            appMenu.items[7].title = "HFNI: \(output[0])"
        }
        addr = 0x31
        if kIOReturnSuccess == IOConnectCallMethod(conf.connect, UInt32(kYSMCUCReadEC), &addr, 1, nil, 0, nil, nil, &output, &outputSize),
           outputSize == 1 {
            if ((output[0] & 0x1) != 0) {
                showOSD("Second Fan!")
                os_log("2nd fan reg: 0x%x", type: .info, output[0])
            }
        }
        #endif
    }

    @objc func setThinkFan(_ sender: NSSlider) {
        print("Val: \(sender.integerValue)")
        text?.stringValue = "\(sender.integerValue)"
        guard appMenu.items[2].title == "Class: ThinkVPC" else {
            showOSD("Val: \(sender.integerValue)")
            return
        }

        var addr : UInt64 = 0x2f // HFSP
        var input : [UInt8] = [sender.intValue == 8 ? 0x80 : UInt8(sender.integerValue)]
        if kIOReturnSuccess != IOConnectCallMethod(conf.connect, UInt32(kYSMCUCWriteEC), &addr, 1, &input, 1, nil, nil, nil, nil) {
            os_log("Write Fan Speed failed!", type: .fault)
            showOSD("Write Fan Speed failed!")
        }
    }

    @objc func updateMuteStatus() {
        if let current = scriptHelper(getMicVolumeAS, "MicMute"),
           sendNumber("MicMuteLED", current.int32Value == 0 ? 2 : 0, conf.io_service) {
            os_log("Mic Mute LED updated", type: .info)
        } else {
            os_log("Failed to update Mic Mute LED", type: .error)
        }
    }

    @objc func toggleStartAtLogin(_ sender: NSMenuItem) {
        if setStartAtLogin(enabled: sender.state == .off) {
            sender.state = (sender.state == .on) ? .off : .on
            defaults.setValue((sender.state == .on), forKey: "StartAtLogin")
        }
    }

    @objc func openPrefpane() {
        prefpaneHelper()
    }

    // from https://medium.com/@hoishing/menu-bar-apps-f2d270150660
    @objc func displayMenu() {
        guard let button = statusItem?.button else { return }
        let x = button.frame.origin.x
        let y = button.frame.origin.y - 5
        let location = button.superview!.convert(NSMakePoint(x, y), to: nil)
        let w = button.window!
        let event = NSEvent.mouseEvent(with: .leftMouseUp,
                                       location: location,
                                       modifierFlags: NSEvent.ModifierFlags(rawValue: 0),
                                       timestamp: 0,
                                       windowNumber: w.windowNumber,
                                       context: w.graphicsContext,
                                       eventNumber: 0,
                                       clickCount: 1,
                                       pressure: 0)!
        NSMenu.popUpContextMenu(appMenu, with: event, for: button)
        if isThink {
            updateThinkFan()
        }
    }

    func applicationDidFinishLaunching(_ aNotification: Notification) {
        guard conf.io_service != 0,
              kIOReturnSuccess == IOServiceOpen(conf.io_service, mach_task_self_, 0, &conf.connect) else {
            os_log("Failed to connect to YogaSMC", type: .fault)
            showOSD("Failed to connect \n to YogaSMC", duration: 2000)
            NSApp.terminate(nil)
            return
        }

        guard kIOReturnSuccess == IOConnectCallScalarMethod(conf.connect, UInt32(kYSMCUCOpen), nil, 0, nil, nil) else {
            os_log("Another instance has connected to YogaSMC", type: .error)
            showOSD("Another instance has \n connected to YogaSMC", duration: 2000)
            NSApp.terminate(nil)
            return
        }

        os_log("Connected to YogaSMC", type: .info)

        loadConfig()
        
        if hide {
            os_log("Icon hidden", type: .info)
        } else {
            statusItem = NSStatusBar.system.statusItem(withLength: -1)
            guard let button = statusItem?.button else {
                os_log("Status bar item failed. Try removing some menu bar item.", type: .fault)
                NSApp.terminate(nil)
                return
            }
            
            button.title = "⎇"
            button.target = self
            button.action = #selector(displayMenu)
        }

        if !getProerty() {
            os_log("YogaSMC unavailable", type: .error)
            showOSD("YogaSMC unavailable", duration: 2000)
            NSApp.terminate(nil)
        }
    }

    func getProerty() -> Bool {
        var CFProps : Unmanaged<CFMutableDictionary>? = nil
        if (kIOReturnSuccess == IORegistryEntryCreateCFProperties(conf.io_service, &CFProps, kCFAllocatorDefault, 0) && CFProps != nil) {
            if let props = CFProps?.takeRetainedValue() as NSDictionary?,
               let IOClass = props["IOClass"] as? NSString {
                switch IOClass {
                case "IdeaVPC":
                    if conf.events.isEmpty {
                        conf.events = IdeaEvents
                    }
                    _ = registerNotification()
                    #if DEBUG
//                    isThink = true
                    #endif
                case "ThinkVPC":
                    if conf.events.isEmpty {
                        conf.events = ThinkEvents
                    }
                    _ = registerNotification()
                    isThink = true
                default:
                    os_log("Unknown class", type: .error)
                    showOSD("Unknown class", duration: 2000)
                }
                if !hide {
                    appMenu.insertItem(NSMenuItem.separator(), at: 1)
                    appMenu.insertItem(withTitle: "Class: \(IOClass)", action: nil, keyEquivalent: "", at: 2)
                    appMenu.insertItem(withTitle: "Build: \(props["YogaSMC,Build"] as? NSString ?? "Unknown")", action: nil, keyEquivalent: "", at: 3)
                    appMenu.insertItem(withTitle: "Version: \(props["YogaSMC,Version"] as? NSString ?? "Unknown")", action: nil, keyEquivalent: "", at: 4)
                    if isThink {
                        appMenu.insertItem(withTitle: "Fan", action: nil, keyEquivalent: "", at: 5)
                        #if DEBUG
                        appMenu.insertItem(withTitle: "HFSP", action: nil, keyEquivalent: "", at: 6)
                        appMenu.insertItem(withTitle: "HFNI", action: nil, keyEquivalent: "", at: 7)
                        let item = NSMenuItem()
                        let slider = NSSlider(value: 0, minValue: 1, maxValue: 8, target: nil, action: #selector(setThinkFan(_:)))
                        slider.numberOfTickMarks = 8
                        slider.allowsTickMarkValuesOnly = true
                        slider.isContinuous = false
                        slider.frame.size.width = 180
                        slider.frame.origin = NSPoint(x: 20, y: 5)
                        let view = NSView(frame: NSRect(x: 0, y: 0, width: slider.frame.width + 50, height: slider.frame.height + 10))
                        view.addSubview(slider)
                        text = NSTextField(frame: NSRect(x: slider.frame.width + 25, y: 0, width: 30, height: slider.frame.height + 5))
                        text?.isEditable = false
                        text?.isSelectable = false
                        text?.isBezeled = false
                        text?.drawsBackground = false
                        text?.font = .systemFont(ofSize: 14)
                        view.addSubview(text!)
                        item.view = view
                        appMenu.insertItem(item, at: 8)
                        #endif
                        updateThinkFan()
                        if appMenu.items[7].title == "HFNI: 7" {
                            os_log("Might be auto mode at startup", type: .info)
                        }
                        updateMuteStatus()
                        NotificationCenter.default.addObserver(self, selector: #selector(updateMuteStatus), name: NSWorkspace.didWakeNotification, object: nil)
                    }
                }
                return true
            }
        }
        return false
    }

    func applicationWillTerminate(_ aNotification: Notification) {
        saveConfig()
        NotificationCenter.default.removeObserver(self)
        if conf.connect != 0 {
            IOServiceClose(conf.connect)
        }
    }

    func loadEvents() {
        if let arr = defaults.object(forKey: "Events") as? [[String: Any]] {
            for v in arr {
                if let id = v["id"] as? UInt32,
                   let events = v["events"] as? [[String: Any]] {
                    var e : Dictionary<UInt32, eventDesc> = [:]
                    for event in events {
                        if let data = event["data"] as? UInt32,
                           let name = event["name"] as? String {
                            let action = event["action"] as? String
                            e[data] = eventDesc(
                                name,
                                event["image"] as? String,
                                action: (action != nil) ? eventAction(rawValue: action!) ?? .nothing : .nothing,
                                display: event["display"] as? Bool ?? true,
                                option: event["option"] as? String)
                        }
                    }
                    conf.events[id] = e
                }
            }
            os_log("Loaded %d events", type: .info, conf.events.capacity)
        } else {
            os_log("Events not found in preference, loading defaults", type: .info)
        }
    }

    func saveEvents() {
        var array : [[String: Any]] = []
        for (k, v) in conf.events {
            var events : [[String: Any]] = []
            var dict : [String: Any] = [:]
            dict["id"] = Int(k)
            for (data, e) in v {
                var event : [String: Any] = [:]
                event["data"] = Int(data)
                event["name"] = e.name
                if let res = e.image {
                    if let path = Bundle.main.resourcePath,
                       res.hasPrefix(path) {
                        event["image"] = res.lastPathComponent
                   } else {
                        event["image"] = res
                   }
                }
                event["action"] = e.action.rawValue
                event["display"] = e.display
                if e.option != nil {
                    event["option"] = e.option
                }
                events.append(event)
            }
            dict["events"] = events
            array.append(dict)
        }
        defaults.setValue(array, forKey: "Events")
    }
    
    func loadConfig() {
        if defaults.object(forKey: "StartAtLogin") == nil {
            os_log("First launch", type: .info)
            defaults.setValue(false, forKey: "HideIcon")
            defaults.setValue(false, forKey: "StartAtLogin")
        }

        hide = defaults.bool(forKey: "HideIcon")

        if !hide {
            let login = NSMenuItem(title: "Start at Login", action: #selector(toggleStartAtLogin(_:)), keyEquivalent: "s")
            login.state = defaults.bool(forKey: "StartAtLogin") ? .on : .off
            appMenu.insertItem(NSMenuItem.separator(), at: 1)
            appMenu.insertItem(login, at: 2)

            let pref = NSMenuItem(title: "Open YogaSMC Preferences…", action: #selector(openPrefpane), keyEquivalent: "p")
            appMenu.insertItem(NSMenuItem.separator(), at: 3)
            appMenu.insertItem(pref, at: 4)
        }

        if !Bundle.main.bundlePath.hasPrefix("/Applications") {
            showOSD("Please move the app \n into Applications")
        }

        if defaults.object(forKey: "AutoBacklight") != nil {
            _ = sendNumber("AutoBacklight", defaults.integer(forKey: "AutoBacklight"), conf.io_service)
        }

        loadEvents()
    }

    func saveConfig() {
        if !conf.events.isEmpty,
           Bundle.main.bundlePath.hasPrefix("/Applications") {
            saveEvents()
        }
        defaults.setValue(getNumber("AutoBacklight", conf.io_service), forKey: "AutoBacklight")
    }

    func registerNotification() -> Bool {
        if conf.connect != 0 {
            // fix warning per https://forums.swift.org/t/swift-5-2-pointers-and-coretext/34862
            var portContext : CFMachPortContext = withUnsafeMutableBytes(of: &conf) { conf in
                CFMachPortContext(version: 0, info: conf.baseAddress, retain: nil, release: nil, copyDescription: nil)
            }
            if let notificationPort = CFMachPortCreate(kCFAllocatorDefault, notificationCallback, &portContext, nil)  {
                if let runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, notificationPort, 0) {
                    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, .defaultMode)
                }
                IOConnectSetNotificationPort(conf.connect, 0, CFMachPortGetPort(notificationPort), 0)
                return true
            } else {
                os_log("Failed to create mach port", type: .error)
            }
        }
        return false
    }
}

// from https://github.com/MonitorControl/MonitorControl
func setStartAtLogin(enabled: Bool) -> Bool {
    let identifier = "\(Bundle.main.bundleIdentifier!)Helper" as CFString
    if SMLoginItemSetEnabled(identifier, enabled) {
        os_log("Toggle start at login state: %{public}@", type: .info, enabled ? "on" : "off")
        return true
    } else {
        os_log("Toggle start at login failed", type: .error)
        return false
    }
}

func notificationCallback(_ port: CFMachPort?, _ msg: UnsafeMutableRawPointer?, _ size: CFIndex, _ info: UnsafeMutableRawPointer?) {
    let conf = info!.assumingMemoryBound(to: sharedConfig?.self)
    if conf.pointee != nil  {
        if let notification = msg?.load(as: SMCNotificationMessage.self) {
            if let events = conf.pointee?.events[notification.event] {
                if let desc = events[notification.data] {
                    eventActuator(desc, notification.data, conf)
                } else if let desc = events[0]{
                    eventActuator(desc, notification.data, conf)
                } else {
                    let name = String(format:"Event 0x%04x", notification.event)
                    showOSD(name)
                    os_log("Event 0x%04x default data not found", type: .error, notification.event)
                }
            } else {
                let name = String(format:"Event 0x%04x", notification.event)
                showOSD(name)
                conf.pointee?.events[notification.event] = [0: eventDesc(name, nil)]
                #if DEBUG
                os_log("Event 0x%04x", type: .debug, notification.event)
                #endif
            }
        } else {
            showOSD("Null Event")
            os_log("Null Event", type: .error)
        }
    } else {
        os_log("Invalid conf", type: .error)
    }
}

func eventActuator(_ desc: eventDesc, _ data: UInt32, _ conf: UnsafePointer<sharedConfig?>) {
    switch desc.action {
    case .nothing:
        #if DEBUG
        os_log("%s: Do nothing", type: .info, desc.name)
        #endif
    case .script:
        if let scpt = desc.option {
            guard scriptHelper(scpt, desc.name) != nil else {
                return
            }
        } else {
            os_log("%s: script not found", type: .error)
            return
        }
    case .airplane:
        airplaneModeHelper()
        return
    case .bluetooth:
        bluetoothHelper()
        return
    case .bluetoothdiscoverable:
        bluetoothDiscoverableHelper()
        return
    case .backlight:
        switch data {
        case 0:
            showOSDRes("Backlight Off", .BacklightOff)
        case 1:
            showOSDRes("Backlight Low", .BacklightLow)
        case 2:
            showOSDRes("Backlight High", .BacklightHigh)
        default:
            showOSDRes("Backlight \(data)", .BacklightLow)
        }
    case .micmute:
        micMuteHelper(conf.pointee!.io_service)
        return
    case .desktop:
        CoreDockSendNotification("com.apple.showdesktop.awake" as CFString, nil)
        return
    case .expose:
        CoreDockSendNotification("com.apple.expose.front.awake" as CFString, nil)
        return
    case .mission:
        CoreDockSendNotification("com.apple.expose.awake" as CFString, nil)
        return
    case .launchpad:
        CoreDockSendNotification("com.apple.launchpad.toggle" as CFString, nil)
        return
    case .prefpane:
        prefpaneHelper()
        return
    case .sleep:
        if desc.display {
            if let img = desc.image {
                showOSD(desc.name, img)
            } else {
                showOSDRes(desc.name, .Sleep)
            }
            sleep(1)
        }
        _ = scriptHelper(desc.option ?? sleepAS, desc.name)
        return
    case .search:
        _ = scriptHelper(desc.option ?? searchAS, desc.name)
        return
    case .siri:
        _ = scriptHelper(desc.option ?? siriAS, desc.name)
        return
    case .spotlight:
        _ = scriptHelper(desc.option ?? spotlightAS, desc.name)
        return
    case .thermal:
        showOSD("Thermal: \(desc.name)")
        os_log("%s: thermal event", type: .info, desc.name)
    case .wireless:
        wirelessHelper()
        return
    default:
        #if DEBUG
        os_log("%s: Not implmented", type: .info, desc.name)
        #endif
    }

    if desc.display {
        showOSD(desc.name, desc.image)
    }
}

struct sharedConfig {
    var connect : io_connect_t = 0
    var events : Dictionary<UInt32, Dictionary<UInt32, eventDesc>> = [:]
    let io_service : io_service_t = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("YogaVPC"))
}
