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
class AppDelegate: NSObject, NSApplicationDelegate, NSMenuDelegate {
    let defaults = UserDefaults(suiteName: "org.zhen.YogaSMC")!
    var conf = sharedConfig()

    var statusItem: NSStatusItem?
    @IBOutlet weak var appMenu: NSMenu!
    var hide = false
    var ECCap = 0

    var isThink = false
    var fanHelper: ThinkFanHelper?
    var fanHelper2: ThinkFanHelper?

    @objc func thinkWakeup() {
        if let current = scriptHelper(getMicVolumeAS, "MicMute"),
           sendNumber("MicMuteLED", current.int32Value == 0 ? 2 : 0, conf.io_service) {
            os_log("Mic Mute LED updated", type: .info)
        } else {
            os_log("Failed to update Mic Mute LED", type: .error)
        }
        if fanHelper != nil {
            fanHelper?.update(true)
        }
        if fanHelper2 != nil {
            fanHelper2?.update(true)
        }
    }

    // MARK: - Menu

    @IBAction func openAboutPanel(_ sender: NSMenuItem) {
        NSApp.activate(ignoringOtherApps: true)
        NSApp.orderFrontStandardAboutPanel(sender)
    }

    @objc func toggleStartAtLogin(_ sender: NSMenuItem) {
        let identifier = "\(Bundle.main.bundleIdentifier!)Helper" as CFString
        if SMLoginItemSetEnabled(identifier, sender.state == .off) {
            sender.state = (sender.state == .on) ? .off : .on
            defaults.setValue((sender.state == .on), forKey: "StartAtLogin")
        } else {
            os_log("Toggle start at login failed", type: .error)
        }
    }

    @objc func openPrefpane() {
        prefpaneHelper()
    }
    
    @objc func midnightTimer(sender: Timer) {
        setHolidayIcon(statusItem!)
        let calendar = Calendar.current
        let midnight = calendar.startOfDay(for: Date())
        sender.fireDate = calendar.date(byAdding: .day, value: 1, to: midnight)!
        os_log("Timer sheduled at %s", type: .info, sender.fireDate.description(with: Locale.current))
    }

    func menuNeedsUpdate(_ menu: NSMenu) {
        if fanHelper != nil {
            fanHelper?.update()
        }
        if fanHelper2 != nil {
            fanHelper2?.update()
        }
    }

    // MARK: - Application

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
            statusItem = NSStatusBar.system.statusItem(withLength: NSStatusItem.variableLength)
            statusItem?.menu = appMenu
            if let title = defaults.string(forKey: "Title") {
                statusItem?.title = title
                statusItem?.toolTip = defaults.string(forKey: "ToolTip")
            } else {
                _ = Timer.scheduledTimer(timeInterval: 0, target: self, selector: #selector(midnightTimer(sender:)), userInfo: nil, repeats: true)
            }
            appMenu.delegate = self
        }

        if !getProerty() {
            os_log("YogaSMC unavailable", type: .error)
            showOSD("YogaSMC unavailable", duration: 2000)
            NSApp.terminate(nil)
        }
    }

    func applicationWillTerminate(_ aNotification: Notification) {
        saveConfig()
        NotificationCenter.default.removeObserver(self)
        if conf.connect != 0 {
            IOServiceClose(conf.connect)
        }
    }

    // MARK: - Configuration

    func getProerty() -> Bool {
        var CFProps : Unmanaged<CFMutableDictionary>? = nil
        if kIOReturnSuccess == IORegistryEntryCreateCFProperties(conf.io_service, &CFProps, kCFAllocatorDefault, 0),
           CFProps != nil {
            if let props = CFProps?.takeRetainedValue() as NSDictionary?,
               let IOClass = props["IOClass"] as? NSString {
                var isOpen = false
                switch getString("EC Capability", conf.io_service) {
                case "RW":
                    ECCap = 3
                case "RO":
                    ECCap = 1
                default:
                    break
                }
                switch IOClass {
                case "IdeaVPC":
                    conf.events = IdeaEvents
                    isOpen = registerNotification()
                case "ThinkVPC":
                    conf.events = ThinkEvents
                    isOpen = registerNotification()
                    isThink = (ECCap != 0) ? true : false
                default:
                    os_log("Unknown class", type: .error)
                    showOSD("Unknown class", duration: 2000)
                }
                if isOpen {
                    loadEvents()
                }
                if !hide {
                    appMenu.insertItem(NSMenuItem.separator(), at: 1)
                    appMenu.insertItem(withTitle: "Class: \(IOClass)", action: nil, keyEquivalent: "", at: 2)
                    appMenu.insertItem(withTitle: "\(props["VersionInfo"] as? NSString ?? "Unknown Version")", action: nil, keyEquivalent: "", at: 3)
                    if isThink {
                        if ECCap == 3 {
                            if !getBoolean("Dual fan", conf.io_service),
                               defaults.bool(forKey: "SecondThinkFan") {
                                fanHelper2 = ThinkFanHelper(appMenu, conf.connect, false, false)
                            }
                            fanHelper = ThinkFanHelper(appMenu, conf.connect, true, fanHelper2 == nil)
                        } else {
                            showOSD("EC access unavailable! \n See `SSDT-ECRW.dsl`")
                        }
                        thinkWakeup()
                        NotificationCenter.default.addObserver(self, selector: #selector(thinkWakeup), name: NSWorkspace.didWakeNotification, object: nil)
                    }
                }
                return true
            }
        }
        return false
    }

    func loadEvents() {
        if let arr = defaults.object(forKey: "Events") as? [[String: Any]] {
            for v in arr {
                if let id = v["id"] as? UInt32,
                   let events = v["events"] as? [[String: Any]] {
                    var e = conf.events[id] ?? [:]
                    for event in events {
                        if let data = event["data"] as? UInt32,
                           let name = event["name"] as? String,
                           let action = event["action"] as? String {
                            let opt = event["option"] as? String
                            if e[data] != nil,
                               action == "nothing",
                               (opt == "Unknown" || (opt == nil && name.hasPrefix("Event 0x"))) {
                                os_log("Override unknown event 0x%04x : %d", type: .info, id, data)
                            } else {
                                e[data] = eventDesc(
                                    name,
                                    event["image"] as? String,
                                    action: eventAction(rawValue: action) ?? .nothing,
                                    display: event["display"] as? Bool ?? true,
                                    option: opt)
                            }
                        } else {
                            os_log("Invalid event for 0x%04x!", type: .info, id)
                        }
                    }
                    conf.events[id] = e
                } else {
                    os_log("Invalid event!", type: .info)
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
            defaults.setValue("⎇", forKey: "Title")
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

        // Load driver settings
        if defaults.object(forKey: "AutoBacklight") != nil {
            _ = sendNumber("AutoBacklight", defaults.integer(forKey: "AutoBacklight"), conf.io_service)
        }
    }

    func saveConfig() {
        if !conf.events.isEmpty,
           Bundle.main.bundlePath.hasPrefix("/Applications") {
            saveEvents()
        }

        // Save driver settings
        defaults.setValue(getNumber("AutoBacklight", conf.io_service), forKey: "AutoBacklight")
    }

    // MARK: - Notification

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

func notificationCallback(_ port: CFMachPort?, _ msg: UnsafeMutableRawPointer?, _ size: CFIndex, _ info: UnsafeMutableRawPointer?) {
    if let raw = info?.assumingMemoryBound(to: sharedConfig?.self),
       var conf = raw.pointee  {
        if let notification = msg?.load(as: SMCNotificationMessage.self) {
            if let events = conf.events[notification.event] {
                if let desc = events[notification.data] {
                    eventActuator(desc, notification.data, &conf)
                } else if let desc = events[0]{
                    eventActuator(desc, notification.data, &conf)
                } else {
                    let name = String(format:"Event 0x%04x", notification.event)
                    showOSD(name)
                    os_log("Event 0x%04x default data not found", type: .error, notification.event)
                }
            } else {
                let name = String(format:"Event 0x%04x", notification.event)
                showOSD(name)
                conf.events[notification.event] = [0: eventDesc(name, nil, option: "Unknown")]
                #if DEBUG
                os_log("Event 0x%04x:%d", type: .debug, notification.event, notification.data)
                #endif
            }
        } else {
            showOSD("Invalid Notification")
            os_log("Invalid notification", type: .error)
        }
    } else {
        os_log("Invalid conf", type: .error)
    }
}

func eventActuator(_ desc: eventDesc, _ data: UInt32, _ conf: UnsafePointer<sharedConfig>) {
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
        micMuteHelper(conf.pointee.io_service)
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
        _ = scriptHelper(sleepAS, desc.name)
        return
    case .search:
        _ = scriptHelper(searchAS, desc.name)
        return
    case .siri:
        _ = scriptHelper(siriAS, desc.name)
        return
    case .spotlight:
        _ = scriptHelper(spotlightAS, desc.name)
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
