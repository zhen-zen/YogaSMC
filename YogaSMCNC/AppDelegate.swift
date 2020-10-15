//
//  AppDelegate.swift
//  YogaSMCNC
//
//  Created by Zhen on 10/11/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import AppKit
import os.log
import ServiceManagement

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate {
    let defaults = UserDefaults(suiteName: "org.zhen.YogaSMC")!
    var conf = sharedConfig()

    var statusItem: NSStatusItem?
    @IBOutlet weak var appMenu: NSMenu!
    @IBOutlet weak var vBuild: NSMenuItem!
    @IBOutlet weak var vVersion: NSMenuItem!
    @IBOutlet weak var vClass: NSMenuItem!
    @IBOutlet weak var vFan: NSMenuItem!

    func updateFan() {
        if conf.connect != 0 {
            var input : UInt64 = 0x84
            var outputSize = 2;
            var output : [UInt8] = Array(repeating: 0, count: 2)
            if kIOReturnSuccess == IOConnectCallMethod(conf.connect, UInt32(kYSMCUCReadEC), &input, 1, nil, 0, nil, nil, &output, &outputSize),
               outputSize == 2 {
                let vFanSpeed = Int32(output[0]) | Int32(output[1]) << 8
                vFan.title = "Fan: \(vFanSpeed) rpm"
            } else {
                os_log("Failed to access EC", type: .error)
            }
        }
    }

    @IBOutlet weak var vStartAtLogin: NSMenuItem!
    @IBAction func toggleStartAtLogin(_ sender: NSMenuItem) {
        if setStartAtLogin(enabled: vStartAtLogin.state == .off) {
            vStartAtLogin.state = (vStartAtLogin.state == .on) ? .off : .on
            defaults.setValue((vStartAtLogin.state == .on), forKey: "StartAtLogin")
        }
    }

    @IBAction func openPrefpane(_ sender: NSMenuItem) {
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
        if vClass.title == "Class: Think" {
            updateFan()
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
        
        if defaults.bool(forKey: "HideIcon") {
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
            if let props = CFProps?.takeRetainedValue() as NSDictionary? {
                let build = props["YogaSMC,Build"] as? NSString
                vBuild.title = "Build: \(build ?? "Unknown")"
                let version = props["YogaSMC,Version"] as? NSString
                vVersion.title = "Version: \(version ?? "Unknown")"
                switch props["IOClass"] as? NSString {
                case "IdeaVPC":
                    if conf.events.isEmpty {
                        conf.events = IdeaEvents
                    }
                    _ = registerNotification()
                    vClass.title = "Class: Idea"
                case "ThinkVPC":
                    if conf.events.isEmpty {
                        conf.events = ThinkEvents
                    }
                    _ = registerNotification()
                    vFan.isHidden = false
                    updateFan()
                    vClass.title = "Class: Think"
                default:
                    vClass.title = "Class: Unknown"
                    os_log("Unknown class", type: .error)
                    showOSD("Unknown class", duration: 2000)
                }
                return true
            }
        }
        return false
    }

    func applicationWillTerminate(_ aNotification: Notification) {
        if conf.connect != 0 {
            IOServiceClose(conf.connect);
        }
        if !conf.events.isEmpty,
           Bundle.main.bundlePath.hasPrefix("/Applications") {
            saveEvents()
        }
    }

    func loadEvents() {
        if !Bundle.main.bundlePath.hasPrefix("/Applications") {
            showOSD("Please move the app \n into Applications")
        }
        if let arr = defaults.object(forKey: "Events") as? [[String: Any]] {
            for v in arr {
                conf.events[v["id"] as! UInt32] = eventDesc(
                    v["name"] as! String,
                    v["image"] as? String,
                    action: eventAction(rawValue: v["action"] as! String) ?? .nothing,
                    display: v["display"] as? Bool ?? true,
                    script: v["script"] as? String)
            }
            os_log("Loaded %d events", type: .info, conf.events.capacity)
        } else {
            os_log("Events not found in preference, loading defaults", type: .info)
        }
    }

    func saveEvents() {
        var array : [[String: Any]] = []
        for (k, v) in conf.events {
            var dict : [String: Any] = [:]
            dict["id"] = Int(k)
            dict["name"] = v.name
            if let res = v.image {
                if let path = Bundle.main.resourcePath,
                   res.hasPrefix(path) {
                    dict["image"] = res.lastPathComponent
               } else {
                    dict["image"] = res
               }
            }
            dict["action"] = v.action.rawValue
            dict["display"] = v.display
            if v.script != nil {
                dict["script"] = v.script
            }
            array.append(dict)
        }
        defaults.setValue(array, forKey: "Events")
        os_log("Default events saved", type: .info)
    }
    
    func loadConfig() {
        if defaults.object(forKey: "FirstLaunch") == nil {
            os_log("First launch", type: .info)
            defaults.setValue(false, forKey: "FirstLaunch")
            defaults.setValue(false, forKey: "HideIcon")
            defaults.setValue(false, forKey: "StartAtLogin")
        }
        vStartAtLogin.state = defaults.bool(forKey: "StartAtLogin") ? .on : .off
        loadEvents()
    }

    func registerNotification() -> Bool {
        if conf.connect != 0 {
            // fix warning per https://forums.swift.org/t/swift-5-2-pointers-and-coretext/34862
            var portContext : CFMachPortContext = withUnsafeMutableBytes(of: &conf) { conf in
                CFMachPortContext(version: 0, info: conf.baseAddress, retain: nil, release: nil, copyDescription: nil)
            }
            if let notificationPort = CFMachPortCreate(kCFAllocatorDefault, notificationCallback, &portContext, nil)  {
                if let runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, notificationPort, 0) {
                    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, .defaultMode);
                }
                IOConnectSetNotificationPort(conf.connect, 0, CFMachPortGetPort(notificationPort), 0);
                return true
            } else {
                os_log("Failed to create mach port", type: .error)
            }
        }
        return false
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
}

func notificationCallback(_ port: CFMachPort?, _ msg: UnsafeMutableRawPointer?, _ size: CFIndex, _ info: UnsafeMutableRawPointer?) {
    let conf = info!.assumingMemoryBound(to: sharedConfig?.self)
    if conf.pointee != nil  {
        if let notification = msg?.load(as: SMCNotificationMessage.self) {
            if let desc = conf.pointee?.events[notification.event] {
                eventActuator(desc, conf)
            } else {
                let name = String(format:"Event 0x%04x", notification.event)
                showOSD(name)
                conf.pointee?.events[notification.event] = eventDesc(name, nil)
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

enum eventAction : String {
    // Userspace
    case nothing, micmute, camera, wireless, bluetooth, prefpane, mirror, spotlight, mission, launchpad, sleep
    // Driver
    case backlit, keyboard, thermal
}

// Resources
enum eventImage : String {
    case kBright//, kBrightOff
}

func eventActuator(_ desc: eventDesc, _ conf: UnsafePointer<sharedConfig?>) {
    var ret : NSAppleEventDescriptor?
    if let src = desc.script {
        if let script = NSAppleScript(source: src) {
            var error: NSDictionary?
            ret = script.executeAndReturnError(&error)
            if error != nil {
                os_log("%s: failed to execute script", type: .error, desc.name)
            }
            if ret!.isRecordDescriptor {
                os_log("%s: valid return", type: .info, desc.name)
            }
        }
    }

    switch desc.action {
    case .nothing:
        #if DEBUG
        os_log("%s: Do nothing", type: .info, desc.name)
        #endif
    case .backlit:
        let backlightLevel = getNumber("BacklightLevel", conf.pointee!.io_service)
        if backlightLevel != -1 {
            // TODO: low / high
            showOSD(desc.name, backlightLevel != 0 ? desc.image : nil)
        } else {
            os_log("%s: failed to evaluate status", type: .info, desc.name)
        }
    case .prefpane:
        prefpaneHelper()
    case .sleep:
        if desc.script == nil,
           let scpt = NSAppleScript(source: sleepAS) {
            var error: NSDictionary?
            ret = scpt.executeAndReturnError(&error)
            if error != nil {
                os_log("%s: failed to execute script", type: .error, desc.name)
            }
        }
    case .thermal:
        showOSD("Thermal: \(desc.name)")
        os_log("%s: thermal event", type: .info, desc.name)
    default:
        #if DEBUG
        os_log("%s: Not implmented", type: .info, desc.name)
        #endif
    }

    if desc.display {
        showOSD(desc.name, desc.image)
    }
}

struct eventDesc {
    let name : String
    let image : NSString?
    let action : eventAction
    let display : Bool
    let script : String?

    init(_ name: String, _ image: String? = nil, action: eventAction = .nothing, display: Bool = true, script: String? = nil) {
        self.name = name
        if let img = image {
            if img.hasPrefix("/") {
                self.image = img as NSString
            } else if let path = Bundle.main.path(forResource: img, ofType: nil),
                      path.hasPrefix("/Applications") {
                self.image = path as NSString
            } else {
                os_log("Incompatible res path: %s", type: .error, img)
                self.image = nil
            }
        } else {
            self.image = nil
        }
        self.action = action
        self.display = display
        self.script = script
    }

    init(_ name: String, _ image: eventImage, action: eventAction = .nothing, display: Bool = true, script: String? = nil) {
        self.name = name
        if let path = Bundle.main.path(forResource: image.rawValue, ofType: "pdf"),
           path.hasPrefix("/Applications") {
            self.image = path as NSString
        } else {
            self.image = nil
        }
        self.action = action
        self.display = display
        self.script = script
    }
}

struct sharedConfig {
    var connect : io_connect_t = 0
    var events : Dictionary<UInt32, eventDesc> = [:]
    let io_service : io_service_t = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("YogaVPC"))
}

let IdeaEvents : Dictionary<UInt32, eventDesc> = [
    0x0040 : eventDesc("Fn-Q Cooling", action: .thermal),
    0x0100 : eventDesc("Keyboard Backlight", "kBright.pdf", action: .backlit, display: false),
    0x0200 : eventDesc("Screen Off"),
    0x0201 : eventDesc("Screen On"),
    0x0500 : eventDesc("TouchPad Off"), // Off
    0x0501 : eventDesc("TouchPad On"), // Same as 0x0A00
    0x0700 : eventDesc("Camera", action: .camera),
    0x0800 : eventDesc("Mic Mute", action: .micmute),
    0x0A00 : eventDesc("TouchPad On", display: false),
    0x0D00 : eventDesc("Airplane Mode", action: .wireless),
]

let ThinkEvents : Dictionary<UInt32, eventDesc> = [
    TP_HKEY_EV_SLEEP.rawValue : eventDesc("Sleep", action: .sleep), // 0x1004
    TP_HKEY_EV_NETWORK.rawValue : eventDesc("Airplane Mode", action: .wireless), // 0x1005
    TP_HKEY_EV_DISPLAY.rawValue : eventDesc("Second Display", action: .mirror), // 0x1007
    TP_HKEY_EV_BRGHT_UP.rawValue : eventDesc("Brightness Up", display: false), // 0x1010
    TP_HKEY_EV_BRGHT_DOWN.rawValue : eventDesc("Brightness Down", display: false), // 0x1011
    TP_HKEY_EV_KBD_LIGHT.rawValue : eventDesc("Keyboard Backlight", "kBright.pdf", action: .backlit, display: false), // 0x1012
    TP_HKEY_EV_MIC_MUTE.rawValue : eventDesc("Mic Mute", action: .micmute), // 0x101B
    TP_HKEY_EV_SETTING.rawValue : eventDesc("Settings", action: .prefpane), // 0x101D
    TP_HKEY_EV_SEARCH.rawValue : eventDesc("Search", action: .spotlight), // 0x101E
    TP_HKEY_EV_MISSION.rawValue : eventDesc("Mission Control", action: .mission), // 0x101F
    TP_HKEY_EV_APPS.rawValue : eventDesc("Launchpad", action: .launchpad), // 0x1020
    TP_HKEY_EV_STAR.rawValue : eventDesc("Custom Hotkey", action: .nothing, script: prefpaneAS), // 0x1311
    TP_HKEY_EV_BLUETOOTH.rawValue : eventDesc("Bluetooth", action: .bluetooth), // 0x1314
    TP_HKEY_EV_KEYBOARD.rawValue : eventDesc("Keyboard Disable", action: .keyboard), // 0x1315
    TP_HKEY_EV_KEY_FN_ESC.rawValue : eventDesc("FnLock"), // 0x6060
]
