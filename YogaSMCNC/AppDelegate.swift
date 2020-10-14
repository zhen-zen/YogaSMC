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
    var conf = notificationConfig()

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
        // from https://medium.com/macoclock/everything-you-need-to-do-to-launch-an-applescript-from-appkit-on-macos-catalina-with-swift-1ba82537f7c3
        let source = """
                            tell application "System Preferences"
                                reveal pane "org.zhen.YogaSMCPane"
                                activate
                            end tell
                     """
        if let script = NSAppleScript(source: source) {
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
        // from https://ffried.codes/2018/01/20/the-internals-of-the-macos-hud/
        let conn = NSXPCConnection(machServiceName: "com.apple.OSDUIHelper", options: [])
        conn.remoteObjectInterface = NSXPCInterface(with: OSDUIHelperProtocol.self)
        conn.interruptionHandler = { os_log("Interrupted!", type: .error) }
        conn.invalidationHandler = { os_log("Invalidated!", type: .error) }
        conn.resume()

        let target = conn.remoteObjectProxyWithErrorHandler { os_log("Failed: %@", type: .error, $0 as CVarArg) }
        conf.helper = target as? OSDUIHelperProtocol
        if conf.helper == nil {
            os_log("Wrong type %@", type: .fault, target as! CVarArg)
            let alert = NSAlert()
            alert.messageText = "Failed to init OSD helper"
            alert.alertStyle = .critical
            alert.addButton(withTitle: "Quit")
            alert.runModal()
            NSApp.terminate(nil)
        }

        conf.io_service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("YogaVPC"))
        if kIOReturnSuccess != IOServiceOpen(conf.io_service, mach_task_self_, 0, &conf.connect) {
            os_log("Failed to connect to YogaSMC", type: .fault)
            conf.helper.showImageAtPath(defaultImage, onDisplayID: CGMainDisplayID(), priority: 0x1f4, msecUntilFade: 2000, withText: "Failed to connect \n to YogaSMC")
            NSApp.terminate(nil)
        } else {
            if kIOReturnSuccess == IOConnectCallScalarMethod(conf.connect, UInt32(kYSMCUCOpen), nil, 0, nil, nil) {
                os_log("Connected to YogaSMC", type: .info)
            } else {
                os_log("Another instance have connected to YogaSMC", type: .error)
                conf.helper.showImageAtPath(defaultImage, onDisplayID: CGMainDisplayID(), priority: 0x1f4, msecUntilFade: 1000, withText: "Another instance have \n connected to YogaSMC")
                NSApp.terminate(nil)
            }
        }

        loadConfig()
        
        if !defaults.bool(forKey: "HideIcon") {
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
            os_log("YogaSMC not installed", type: .error)
            conf.helper.showImageAtPath(defaultImage, onDisplayID: CGMainDisplayID(), priority: 0x1f4, msecUntilFade: 1000, withText: "YogaSMC Unavailable")
            NSApp.terminate(nil)
        }
    }

    func getProerty() -> Bool {
        if conf.io_service != 0 {
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
                        conf.helper.showImageAtPath(defaultImage, onDisplayID: CGMainDisplayID(), priority: 0x1f4, msecUntilFade: 1000, withText: "Unknown class")
                    }
                    return true
                }
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
            conf.helper.showImageAtPath(defaultImage, onDisplayID: CGMainDisplayID(), priority: 0x1f4, msecUntilFade: 2000, withText: "Please move the app \n into Applications")
        }
        if let arr = defaults.object(forKey: "Events") as? [[String: Any]] {
            for v in arr {
                conf.events[v["id"] as! UInt32] = eventDesc(v["name"] as! String, v["image"] as? String, eventAction(rawValue: v["action"] as! Int) ?? .nothing)
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
            if v.image != nil {
                dict["image"] = v.image
            }
            dict["action"] = v.action.rawValue
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
            var portContext = CFMachPortContext(version: 0, info: &conf, retain: nil, release: nil, copyDescription: nil)
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

let defaultImage : NSString = "/System/Library/CoreServices/OSDUIHelper.app/Contents/Resources/kBrightOff.pdf"

// from https://ffried.codes/2018/01/20/the-internals-of-the-macos-hud/
@objc enum OSDImage: CLongLong {
    case kBrightness = 1
    case brightness2 = 2
    case kVolume = 3
    case kMute = 4
    case volume5 = 5
    case kEject = 6
    case brightness7 = 7
    case brightness8 = 8
    case kAirportRange = 9
    case wireless2Forbid = 10
    case kBright = 11
    case kBrightOff = 12
    case kBright13 = 13
    case kBrightOff14 = 14
    case Ajar = 15
    case mute16 = 16
    case volume17 = 17
    case empty18 = 18
    case kRemoteLinkedGeneric = 19
    case kRemoteSleepGeneric = 20 // will put into sleep
    case muteForbid = 21
    case volumeForbid = 22
    case volume23 = 23
    case empty24 = 24
    case kBright25 = 25
    case kBrightOff26 = 26
    case backlightonForbid = 27
    case backlightoffForbid = 28
    /* and more cases from 1 to 28 (except 18 and 24) */
}

// from https://github.com/w0lfschild/macOS_headers/blob/master/macOS/CoreServices/OSDUIHelper/1/OSDUIHelperProtocol-Protocol.h

@objc protocol OSDUIHelperProtocol {

    @objc func showFullScreenImage(_ img: OSDImage, onDisplayID: CGDirectDisplayID, priority: CUnsignedInt, msecToAnimate: CUnsignedInt)

    @objc func fadeClassicImageOnDisplay(_ img: CUnsignedInt)

    @objc func showImageAtPath(_ img: NSString, onDisplayID: CGDirectDisplayID, priority: CUnsignedInt, msecUntilFade: CUnsignedInt, withText: NSString)

    @objc func showImage(_ img: OSDImage, onDisplayID: CGDirectDisplayID, priority: CUnsignedInt, msecUntilFade: CUnsignedInt, filledChiclets: CUnsignedInt, totalChiclets: CUnsignedInt, locked: CBool)

    @objc func showImage(_ img: OSDImage, onDisplayID: CGDirectDisplayID, priority: CUnsignedInt, msecUntilFade: CUnsignedInt, withText: NSString)

    @objc func showImage(_ img: OSDImage, onDisplayID: CGDirectDisplayID, priority: CUnsignedInt, msecUntilFade: CUnsignedInt)
}

func notificationCallback(_ port: CFMachPort?, _ msg: UnsafeMutableRawPointer?, _ size: CFIndex, _ info: UnsafeMutableRawPointer?) {
    let conf = info!.assumingMemoryBound(to: notificationConfig?.self)
    if conf.pointee != nil  {
        if let notification = msg?.load(as: SMCNotificationMessage.self) {
            if let desc = conf.pointee?.events[notification.event] {
                eventActuator(desc, conf)
            } else {
                let name = String(format:"Event 0x%04x", notification.event)
                conf.pointee?.helper.showImageAtPath(defaultImage, onDisplayID: CGMainDisplayID(), priority: 0x1f4, msecUntilFade: 1000, withText: name as NSString)
                conf.pointee?.events[notification.event] = eventDesc(name, nil)
                #if DEBUG
                os_log("Event 0x%04x", type: .debug, notification.event)
                #endif
            }
        } else {
            conf.pointee?.helper.showImageAtPath(defaultImage, onDisplayID: CGMainDisplayID(), priority: 0x1f4, msecUntilFade: 1000, withText: "Null Event")
            os_log("Null Event", type: .error)
        }
    } else {
        os_log("Invalid conf", type: .error)
    }
}

enum eventAction : Int {
    case nothing, micMute
}

func eventActuator(_ desc: eventDesc, _ conf: UnsafePointer<notificationConfig?>) {
    conf.pointee?.helper.showImageAtPath(desc.image ?? defaultImage, onDisplayID: CGMainDisplayID(), priority: 0x1f4, msecUntilFade: 1000, withText: desc.name)
    switch desc.action {
    case .nothing:
        os_log("%s: Do nothing", type: .info, desc.name)
    case .micMute:
        os_log("%s: Toggle Mic mute", type: .info, desc.name)
    }
}

struct eventDesc {
    let name : NSString
    let image : NSString?
    let action : eventAction
    init(_ name: String, _ image: String?, _ action: eventAction = .nothing) {
        self.name = name as NSString
        if let path = Bundle.main.resourcePath,
           path.hasPrefix("/Applications"),
            let img = image {
            self.image = "\(path)/\(img)" as NSString
        } else {
            self.image = nil
        }
        self.action = action
    }
}

struct notificationConfig {
    var connect : io_connect_t = 0
    var events : Dictionary<UInt32, eventDesc> = [:]
    var helper : OSDUIHelperProtocol!
    var io_service : io_service_t = 0
}

let IdeaEvents : Dictionary<UInt32, eventDesc> = [0x2 : eventDesc("Keyboard Backlight", "kBright.pdf"), 0x100 : eventDesc("Mic Mute", nil, .micMute)]
let ThinkEvents : Dictionary<UInt32, eventDesc> = [0x1005 : eventDesc("Network", nil), 0x1012 : eventDesc("Keyboard Backlight", "kBright.pdf"), 0x101B : eventDesc("Mic Mute", nil)]
