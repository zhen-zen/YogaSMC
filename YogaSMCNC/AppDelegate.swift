//
//  AppDelegate.swift
//  YogaSMCNC
//
//  Created by Zhen on 10/11/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import AppKit
import ApplicationServices
import Carbon
import NotificationCenter
import os.log
import ServiceManagement

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate, NSMenuDelegate {
    let defaults = UserDefaults(suiteName: "org.zhen.YogaSMC")!
    var conf = SharedConfig()
    var IOClass = "YogaSMC"

    var statusItem: NSStatusItem?
    @IBOutlet weak var appMenu: NSMenu!
    var hide = false
    var hideCapslock = false

    // MARK: - Think

    var fanHelper: ThinkFanHelper?
    var fanHelper2: ThinkFanHelper?
    var fanTimer: Timer?

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

    @objc func iconWakeup() {
        setHolidayIcon(statusItem!)
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
        var iter: io_iterator_t = 0

        IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching("YogaVPC"), &iter)

        while let service = IOIteratorNextOptional(iter) {
            guard let IOClass = getString("IOClass", service) else {
                os_log("Invalid YogaVPC instance", type: .info)
                continue
            }

            os_log("Found %s", type: .info, IOClass)

            var connect: io_connect_t = 0

            guard kIOReturnSuccess == IOServiceOpen(service, mach_task_self_, 0, &connect) else { continue }

            conf.service = service
            self.IOClass = IOClass

            guard kIOReturnSuccess == IOConnectCallScalarMethod(connect, UInt32(kYSMCUCOpen), nil, 0, nil, nil) else {
                IOServiceClose(connect)
                continue
            }

            conf.connect = connect
            break
        }

        guard conf.service != 0 else {
            os_log("Failed to connect to YogaSMC", type: .fault)
            showOSD("ConnectFail", duration: 2000)
            NSApp.terminate(nil)
            return
        }

        guard let props = getProperties(conf.service) else {
            os_log("YogaSMC unavailable", type: .error)
            showOSD("YogaSMC Unavailable", duration: 2000)
            NSApp.terminate(nil)
            return
        }

        guard conf.connect != 0 else {
            os_log("Another instance has connected to %s", type: .fault, IOClass)
            showOSD("AlreadyConnected", duration: 2000)
            NSApp.terminate(nil)
            return
        }

        os_log("Connected to %s", type: .info, IOClass)

        loadConfig()

        initMenu(props["VersionInfo"] as? String ?? NSLocalizedString("Unknown Version", comment: ""))
        initNotification(props["EC Capability"] as? String)

        if GetCurrentKeyModifiers() & UInt32(alphaLock) != 0 {
            conf.modifier.insert(.capsLock)
        }
        NSEvent.addGlobalMonitorForEvents(matching: .flagsChanged, handler: flagsCallBack)
    }

    func applicationWillTerminate(_ aNotification: Notification) {
        saveConfig()
        NSWorkspace.shared.notificationCenter.removeObserver(self)
        IOServiceClose(conf.connect)
    }

    // MARK: - Configuration

    func initMenu(_ version: String) {
        if hide {
            os_log("Icon hidden", type: .info)
            return
        }

        statusItem = NSStatusBar.system.statusItem(withLength: NSStatusItem.variableLength)
        statusItem?.menu = appMenu
        if let title = defaults.string(forKey: "Title") {
            statusItem?.title = title
            statusItem?.toolTip = defaults.string(forKey: "ToolTip")
        } else {
            _ = Timer.scheduledTimer(timeInterval: 0, target: self, selector: #selector(midnightTimer(sender:)),
                                     userInfo: nil, repeats: true)
            NSWorkspace.shared.notificationCenter.addObserver(self, selector: #selector(iconWakeup),
                                                              name: NSWorkspace.didWakeNotification, object: nil)
        }

        appMenu.delegate = self

        let classPrompt = NSLocalizedString("ClassVar", comment: "Class: ") + IOClass
        appMenu.insertItem(NSMenuItem.separator(), at: 1)
        appMenu.insertItem(withTitle: classPrompt, action: nil, keyEquivalent: "", at: 2)
        appMenu.insertItem(withTitle: version, action: nil, keyEquivalent: "", at: 3)

        let loginPrompt = NSLocalizedString("Start at Login", comment: "")
        let login = NSMenuItem(title: loginPrompt, action: #selector(toggleStartAtLogin), keyEquivalent: "s")
        login.state = defaults.bool(forKey: "StartAtLogin") ? .on : .off
        appMenu.insertItem(NSMenuItem.separator(), at: 4)
        appMenu.insertItem(login, at: 5)

        let prefPrompt = NSLocalizedString("Preferences", comment: "")
        let pref = NSMenuItem(title: prefPrompt, action: #selector(openPrefpane), keyEquivalent: "p")
        appMenu.insertItem(NSMenuItem.separator(), at: 6)
        appMenu.insertItem(pref, at: 7)
    }

    func initNotification(_ ECCap: String?) {
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
                if ECCap != "RW" {
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
            NSWorkspace.shared.notificationCenter.addObserver(self, selector: #selector(thinkWakeup),
                                                              name: NSWorkspace.didWakeNotification, object: nil)
        case "YogaHIDD":
            conf.events = HIDDEvents
            _ = registerNotification()
            os_log("Skip loadEvents", type: .info)
        default:
            os_log("Unknown class", type: .error)
            showOSD("Unknown Class", duration: 2000)
        }
        if isOpen {
            loadEvents()
        }
    }

    func flagsCallBack(event: NSEvent) {
//        print(event.keyCode)
//        switch event.keyCode {
//        case 0x36:
//            os_log("left_option", type: .debug)
//        case 0x37:
//            os_log("right_option", type: .debug)
//        case 0x39:
//            os_log("caps_lock", type: .debug)
//        case 0x3b:
//            os_log("left_control", type: .debug)
//        case 0x3d:
//            os_log("left_command", type: .debug)
//        case 0x3e:
//            os_log("right_control", type: .debug)
//        default:
//            os_log("Unknown %x", type: .debug, event.keyCode)
//        }
        // 1 << 16
        if !hideCapslock {
            if event.modifierFlags.contains(.capsLock) {
                if !conf.modifier.contains(.capsLock) {
                    showOSDRes("Caps Lock", "On", .kCapslockOn)
                }
            } else if conf.modifier.contains(.capsLock) {
                showOSDRes("Caps Lock", "Off", .kCapslockOff)
            }
        }
        conf.modifier = event.modifierFlags.intersection(.deviceIndependentFlagsMask)
        // 1 << 17
//        if conf.modifier.contains(.shift) {
//            os_log("Shift", type: .debug)
//            flags.remove(.shift)
//        }
        // 1 << 18
//        if conf.modifier.contains(.control) {
//            os_log("Control", type: .debug)
//            flags.remove(.control)
//        }
        // 1 << 19
//        if conf.modifier.contains(.option) {
//            os_log("Option", type: .debug)
//            flags.remove(.option)
//        }
        // 1 << 20
//        if conf.modifier.contains(.command) {
//            os_log("Command", type: .debug)
//            flags.remove(.command)
//        }
//        if !flags.isEmpty {
//            print(flags)
//        }
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
            defaults.setValue(false, forKey: "StartAtLogin")
        }

        hide = defaults.bool(forKey: "HideIcon")
        hideCapslock = defaults.bool(forKey: "HideCapsLock")

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
           Bundle.main.bundlePath.hasPrefix("/Applications"),
           IOClass != "YogaHIDD" {
            saveEvents()
        }

        // Save driver settings
        defaults.setValue(getNumber("AutoBacklight", conf.service), forKey: "AutoBacklight")
    }

    // MARK: - Notification

    func registerNotification() -> Bool {
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
}

func notificationCallback(_ port: CFMachPort?, _ msg: UnsafeMutableRawPointer?, _ size: CFIndex, _ info: UnsafeMutableRawPointer?) {
    if let raw = info?.assumingMemoryBound(to: SharedConfig?.self),
       var conf = raw.pointee {
        if let notification = msg?.load(as: SMCNotificationMessage.self) {
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
                let prompt = NSLocalizedString("EventVar", comment: "Event ") + desc.name.dropFirst("Event ".count)
                showOSDRaw(prompt, desc.image)
            } else {
                showOSD(desc.name, desc.image)
            }
        }
    case .script:
        if let scpt = desc.option {
            _ = scriptHelper(scpt, desc.name, desc.display ? desc.image : nil)
        } else {
            os_log("%s: script not found", type: .error)
        }
    case .launchapp:
        if let name = desc.option {
            let scpt = "tell application \"" + name + "\" to activate"
            _ = scriptHelper(scpt, desc.name, desc.display ? desc.image : nil)
        } else {
            os_log("%s: script not found", type: .error)
        }
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
        if data == 0 {
            showOSDRes("Keyboard", "Disabled", .kKeyboardOff)
        } else {
            showOSDRes("Keyboard", "Enabled", .kKeyboard)
        }
    case .micmute:
        micMuteHelper(conf.pointee.service, desc.name)
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
            if let img = desc.image {
                showOSD(desc.name, img)
            } else {
                showOSDRes(desc.name, .kSleep)
            }
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

struct SharedConfig {
    var connect: io_connect_t = 0
    var events: [UInt32: [UInt32: EventDesc]] = [:]
    var modifier = NSEvent.ModifierFlags()
    var service: io_service_t = 0
}
