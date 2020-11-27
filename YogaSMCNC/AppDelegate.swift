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
    var conf = SharedConfig()

    var statusItem: NSStatusItem?
    @IBOutlet weak var appMenu: NSMenu!
    var hide = false
    var ECCap = 0
    var fanTimer: Timer?

    // MARK: - Think

    var fanHelper: ThinkFanHelper?
    var fanHelper2: ThinkFanHelper?

    @objc func thinkWakeup() {
        if let current = scriptHelper(getMicVolumeAS, "MicMute"),
           sendNumber("MicMuteLED", current.int32Value == 0 ? 2 : 0, conf.service) {
            os_log("Mic Mute LED updated", type: .info)
        } else {
            os_log("Failed to update Mic Mute LED", type: .error)
        }

        if fanHelper2 != nil {
            fanHelper2?.setFanLevel()
        }
        if fanHelper != nil {
            fanHelper?.setFanLevel()
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

    @objc func update () {
        DispatchQueue.main.async {
            if self.fanHelper2 != nil {
                self.fanHelper2!.update()
            }
            if self.fanHelper != nil {
                self.fanHelper!.update()
            }
        }
    }

    func menuWillOpen(_ menu: NSMenu) {
        update()
        DispatchQueue.global(qos: .default).async {
            self.fanTimer = Timer.scheduledTimer(
                timeInterval: 1,
                target: self,
                selector: #selector(self.update),
                userInfo: nil,
                repeats: true)

            if self.fanTimer == nil {
                return
            }
            self.fanTimer?.tolerance = 0.2
            let currentRunLoop = RunLoop.current
            currentRunLoop.add(self.fanTimer!, forMode: .common)
            currentRunLoop.run()
        }
    }

    func menuDidClose(_ menu: NSMenu) {
        self.fanTimer?.invalidate()
    }

    // MARK: - Application

    func applicationDidFinishLaunching(_ aNotification: Notification) {
        guard conf.service != 0,
              kIOReturnSuccess == IOServiceOpen(conf.service, mach_task_self_, 0, &conf.connect) else {
            os_log("Failed to connect to YogaSMC", type: .fault)
            showOSD("ConnectFail", duration: 2000)
            NSApp.terminate(nil)
            return
        }

        guard kIOReturnSuccess == IOConnectCallScalarMethod(conf.connect, UInt32(kYSMCUCOpen), nil, 0, nil, nil) else {
            os_log("Another instance has connected to YogaSMC", type: .error)
            showOSD("AlreadyConnected", duration: 2000)
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
            showOSD("YogaSMC Unavailable", duration: 2000)
            NSApp.terminate(nil)
        }
    }

    func applicationWillTerminate(_ aNotification: Notification) {
        saveConfig()
        NSWorkspace.shared.notificationCenter.removeObserver(self)
        if conf.connect != 0 {
            IOServiceClose(conf.connect)
        }
    }

    // MARK: - Configuration

    func getProerty() -> Bool {
        var CFProps: Unmanaged<CFMutableDictionary>?
        if kIOReturnSuccess == IORegistryEntryCreateCFProperties(conf.service, &CFProps, kCFAllocatorDefault, 0),
           CFProps != nil {
            if let props = CFProps?.takeRetainedValue() as NSDictionary?,
               let IOClass = props["IOClass"] as? NSString {
                if !hide {
                    appMenu.insertItem(NSMenuItem.separator(), at: 1)
                    appMenu.insertItem(withTitle: "Class: \(IOClass)", action: nil, keyEquivalent: "", at: 2)
                    appMenu.insertItem(withTitle: props["VersionInfo"] as? String ?? NSLocalizedString("Unknown Version", comment: ""), action: nil, keyEquivalent: "", at: 3)
                }

                switch getString("EC Capability", conf.service) {
                case "RW":
                    ECCap = 3
                case "RO":
                    ECCap = 1
                default:
                    break
                }

                var isOpen = false
                switch IOClass {
                case "IdeaVPC":
                    conf.events = ideaEvents
                    isOpen = registerNotification()
                case "ThinkVPC":
                    conf.events = thinkEvents
                    isOpen = registerNotification()
                    thinkWakeup()
                    if !hide, !defaults.bool(forKey: "DisableFan") {
                        if ECCap == 3 {
                            if !getBoolean("Dual fan", conf.service),
                               defaults.bool(forKey: "SecondThinkFan") {
                                fanHelper2 = ThinkFanHelper(appMenu, conf.connect, false, false)
                                fanHelper2?.update(true)
                            }
                            fanHelper = ThinkFanHelper(appMenu, conf.connect, true, fanHelper2 == nil)
                            fanHelper?.update(true)
                        } else {
                            showOSD("ECAccessUnavailable")
                        }
                    }
                    NSWorkspace.shared.notificationCenter.addObserver(self, selector: #selector(thinkWakeup), name: NSWorkspace.didWakeNotification, object: nil)
                case "YogaHIDD":
                    conf.events = HIDDEvents
                    isOpen = registerNotification()
                default:
                    os_log("Unknown class", type: .error)
                    showOSD("Unknown Class", duration: 2000)
                }
                if isOpen {
                    loadEvents()
                }
                return true
            }
        }
        return false
    }

    func loadEvents() {
        if let arr = defaults.object(forKey: "Events") as? [[String: Any]] {
            for entry in arr {
                if let eid = entry["id"] as? UInt32,
                   let events = entry["events"] as? [[String: Any]] {
                    var info = conf.events[eid] ?? [:]
                    for event in events {
                        if let data = event["data"] as? UInt32,
                           let name = event["name"] as? String,
                           let action = event["action"] as? String {
                            let opt = event["option"] as? String
                            if info[data] != nil,
                               action == "nothing",
                               (opt == "Unknown" || (opt == nil && name.hasPrefix("Event 0x"))) {
                                os_log("Override unknown event 0x%04x : %d", type: .info, eid, data)
                            } else {
                                info[data] = EventDesc(
                                    name,
                                    event["image"] as? String,
                                    act: EventAction(rawValue: action) ?? .nothing,
                                    display: event["display"] as? Bool ?? true,
                                    opt: opt)
                            }
                        } else {
                            os_log("Invalid event for 0x%04x!", type: .info, eid)
                        }
                    }
                    conf.events[eid] = info
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
        var arr: [[String: Any]] = []
        for (eid, info) in conf.events {
            var events: [[String: Any]] = []
            var entry: [String: Any] = [:]
            entry["id"] = Int(eid)
            for (data, desc) in info {
                var event: [String: Any] = [:]
                event["data"] = Int(data)
                event["name"] = desc.name
                if let res = desc.image {
                    if let path = Bundle.main.resourcePath,
                       res.hasPrefix(path) {
                        event["image"] = res.lastPathComponent
                   } else {
                        event["image"] = res
                   }
                }
                event["action"] = desc.action.rawValue
                event["display"] = desc.display
                if desc.option != nil {
                    event["option"] = desc.option
                }
                events.append(event)
            }
            entry["events"] = events
            arr.append(entry)
        }
        defaults.setValue(arr, forKey: "Events")
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
            let login = NSMenuItem(title: "Start at Login", action: #selector(toggleStartAtLogin), keyEquivalent: "s")
            login.state = defaults.bool(forKey: "StartAtLogin") ? .on : .off
            appMenu.insertItem(NSMenuItem.separator(), at: 1)
            appMenu.insertItem(login, at: 2)

            let pref = NSMenuItem(title: "Open YogaSMC Preferences…", action: #selector(openPrefpane), keyEquivalent: "p")
            appMenu.insertItem(NSMenuItem.separator(), at: 3)
            appMenu.insertItem(pref, at: 4)
        }

        if !Bundle.main.bundlePath.hasPrefix("/Applications") {
            showOSD("MoveApp")
        }

        // Load driver settings
        if defaults.object(forKey: "AutoBacklight") != nil {
            _ = sendNumber("AutoBacklight", defaults.integer(forKey: "AutoBacklight"), conf.service)
        }
    }

    func saveConfig() {
        if !conf.events.isEmpty,
           Bundle.main.bundlePath.hasPrefix("/Applications") {
            saveEvents()
        }

        // Save driver settings
        defaults.setValue(getNumber("AutoBacklight", conf.service), forKey: "AutoBacklight")
    }

    // MARK: - Notification

    func registerNotification() -> Bool {
        if conf.connect != 0 {
            // fix warning per https://forums.swift.org/t/swift-5-2-pointers-and-coretext/34862
            var portContext: CFMachPortContext = withUnsafeMutableBytes(of: &conf) { conf in
                CFMachPortContext(version: 0, info: conf.baseAddress, retain: nil, release: nil, copyDescription: nil)
            }
            if let notificationPort = CFMachPortCreate(kCFAllocatorDefault, notificationCallback, &portContext, nil) {
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
    if let raw = info?.assumingMemoryBound(to: SharedConfig?.self),
       var conf = raw.pointee {
        if let notification = msg?.load(as: SMCNotificationMessage.self) {
            if let events = conf.events[notification.event] {
                if let desc = events[notification.data] {
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
        } else {
            os_log("Invalid notification", type: .error)
        }
    } else {
        os_log("Invalid conf", type: .error)
    }
}

func eventActuator(_ desc: EventDesc, _ data: UInt32, _ conf: UnsafePointer<SharedConfig>) {
    switch desc.action {
    case .nothing:
        #if DEBUG
        os_log("%s: Do nothing", type: .info, desc.name)
        #endif
        if desc.display {
            if desc.name.hasPrefix("Event ") {
                showOSD("\(NSLocalizedString("EventVar", comment: "Event "))\(desc.name.dropFirst("Event ".count))", desc.image)
            } else {
                showOSD("\(NSLocalizedString("EventVar", comment: "Event "))\(desc.name)", desc.image)
            }
        }
        return
    case .script:
        if let scpt = desc.option {
            guard scriptHelper(scpt, desc.name) != nil else { return }
        } else {
            os_log("%s: script not found", type: .error)
            return
        }
    case .airplane:
        airplaneModeHelper(desc.name)
        return
    case .bluetooth:
        bluetoothHelper(desc.name)
        return
    case .bluetoothdiscoverable:
        bluetoothDiscoverableHelper(desc.name)
        return
    case .backlight:
        switch data {
        case 0:
            showOSDRes("Backlight", "Off", .kBacklightOff)
        case 1:
            showOSDRes("Backlight", "Low", .kBacklightLow)
        case 2:
            showOSDRes("Backlight", "High", .kBacklightHigh)
        default:
            showOSDRes("Backlight", "\(data)", .kBacklightLow)
        }
    case .micmute:
        micMuteHelper(conf.pointee.service, desc.name)
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
                showOSDRes(desc.name, .kSleep)
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
        showOSD(desc.name, desc.image)
        os_log("%s: thermal event", type: .info, desc.name)
    case .wireless:
        wirelessHelper(desc.name)
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

struct SharedConfig {
    var connect: io_connect_t = 0
    var events: [UInt32: [UInt32: EventDesc]] = [:]
    let service: io_service_t = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("YogaVPC"))
}
