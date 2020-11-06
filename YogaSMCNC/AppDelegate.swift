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
    var fanLevel: NSTextField?
    var secondThinkFan = false
    var ThinkFanSpeed = "HFSP" // 0x2f
//    var ThinkFanStatus : UInt64  = 0x2f
    var ThinkFanSelect : UInt64  = 0x31
    var ThinkFanRPM : UInt64  = 0x84

    func updateThinkFan() {
        guard conf.connect != 0 else {
            return
        }

        var outputSize = 2
        var output : [UInt8] = [0, 0]
        guard kIOReturnSuccess == IOConnectCallMethod(conf.connect, UInt32(kYSMCUCReadEC), &ThinkFanRPM, 1, nil, 0, nil, nil, &output, &outputSize),
           outputSize == 2 else {
            os_log("Failed to access EC", type: .error)
            return
        }

        appMenu.items[4].title = "Fan: \(Int32(output[0]) | Int32(output[1]) << 8) rpm"

        #if DEBUG
        outputSize = 1

        if kIOReturnSuccess == IOConnectCallMethod(conf.connect, UInt32(kYSMCUCReadECName), nil, 0, &ThinkFanSpeed, 4, nil, nil, &output, &outputSize) {
            appMenu.items[5].title = "HFSP: \(output[0])"
        }

        var name = "HFNI" // 0x83
        if kIOReturnSuccess == IOConnectCallMethod(conf.connect, UInt32(kYSMCUCReadECName), nil, 0, &name, 4, nil, nil, &output, &outputSize) {
            appMenu.items[6].title = "HFNI: \(output[0])"
        }

        guard (ECCap == 3), secondThinkFan else {
            return
        }

        guard kIOReturnSuccess == IOConnectCallMethod(conf.connect, UInt32(kYSMCUCReadEC), &ThinkFanSelect, 1, nil, 0, nil, nil, &output, &outputSize),
           outputSize == 1 else {
            os_log("Failed to read current fan", type: .error)
            return
        }
        os_log("2nd fan reg: 0x%x", type: .info, output[0])

        var input : [UInt8] = [0];
        if (output[0] & 0x1) != 0 {
            input[0] = output[0] & 0xfe
        } else {
            input[0] = output[0] | 0x1
        }

        guard kIOReturnSuccess == IOConnectCallMethod(conf.connect, UInt32(kYSMCUCWriteEC), &ThinkFanSelect, 1, &input, 1, nil, nil, nil, nil) else {
            os_log("Failed to select second fan", type: .error)
            secondThinkFan = false
            return
        }

        outputSize = 2
        if kIOReturnSuccess == IOConnectCallMethod(conf.connect, UInt32(kYSMCUCReadEC), &ThinkFanRPM, 1, nil, 0, nil, nil, &output, &outputSize),
           outputSize == 2 {
            appMenu.items[7].title = "Fan2: \(Int32(output[0]) | Int32(output[1]) << 8) rpm"
        } else {
            os_log("Failed to access second fan", type: .error)
            secondThinkFan = false
        }

        if (input[0] & 0x1) != 0 {
            input[0] &= 0xfe
        } else {
            input[0] |= 0x1
        }

        guard kIOReturnSuccess == IOConnectCallMethod(conf.connect, UInt32(kYSMCUCWriteEC), &ThinkFanSelect, 1, &input, 1, nil, nil, nil, nil) else {
            os_log("Failed to select first fan", type: .error)
            secondThinkFan = false
            return
        }

        #endif
    }

    @objc func setThinkFan(_ sender: NSSlider) {
        print("Val: \(sender.integerValue)")
        fanLevel?.stringValue = "\(sender.integerValue)"
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
        if isThink {
            updateThinkFan()
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
                        updateMuteStatus()
                        NotificationCenter.default.addObserver(self, selector: #selector(updateMuteStatus), name: NSWorkspace.didWakeNotification, object: nil)
                        if defaults.bool(forKey: "SecondThinkFan"),
                           !getBoolean("Dual fan", conf.io_service) {
                            secondThinkFan = true
                        }
                        appMenu.insertItem(withTitle: "Fan", action: nil, keyEquivalent: "", at: 4)
                        #if DEBUG
                        appMenu.insertItem(withTitle: "HFSP", action: nil, keyEquivalent: "", at: 5)
                        appMenu.insertItem(withTitle: "HFNI", action: nil, keyEquivalent: "", at: 6)
                        #endif
                        updateThinkFan()
                        #if DEBUG
                        if (ECCap != 3) {
                            showOSD("EC write unsupported! \n See `SSDT-ECRW.dsl`")
                            return true
                        }
                        if secondThinkFan {
                            appMenu.insertItem(withTitle: "Fan2", action: nil, keyEquivalent: "", at: 7)
                        }
                        let item = NSMenuItem()
                        var slider : NSSlider!
                        if defaults.bool(forKey: "AllowFanStop") {
                            slider = NSSlider(value: 0, minValue: 1, maxValue: 9, target: nil, action: #selector(setThinkFan(_:)))
                            slider.numberOfTickMarks = 9
                        } else {
                            slider = NSSlider(value: 0, minValue: 1, maxValue: 8, target: nil, action: #selector(setThinkFan(_:)))
                            slider.numberOfTickMarks = 8
                        }
                        slider.allowsTickMarkValuesOnly = true
                        slider.isContinuous = false
                        slider.frame.size.width = 180
                        slider.frame.origin = NSPoint(x: 20, y: 5)
                        let view = NSView(frame: NSRect(x: 0, y: 0, width: slider.frame.width + 50, height: slider.frame.height + 10))
                        view.addSubview(slider)
                        fanLevel = NSTextField(frame: NSRect(x: slider.frame.width + 25, y: 0, width: 30, height: slider.frame.height + 5))
                        fanLevel?.isEditable = false
                        fanLevel?.isSelectable = false
                        fanLevel?.isBezeled = false
                        fanLevel?.drawsBackground = false
                        fanLevel?.font = .systemFont(ofSize: 14)
                        view.addSubview(fanLevel!)
                        item.view = view
                        appMenu.insertItem(item, at: 8)
                        if appMenu.items[6].title == "HFNI: 7" {
                            os_log("Might be auto mode at startup", type: .info)
                        }
                        #endif
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
                conf.pointee?.events[notification.event] = [0: eventDesc(name, nil, option: "Unknown")]
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
