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
    var io_service : io_service_t = 0
    var connect : io_connect_t = 0;

    var statusItem: NSStatusItem?
    @IBOutlet weak var appMenu: NSMenu!
    @IBOutlet weak var vBuild: NSMenuItem!
    @IBOutlet weak var vVersion: NSMenuItem!
    @IBOutlet weak var vClass: NSMenuItem!
    @IBOutlet weak var vFan: NSMenuItem!
    @IBAction func updateFan(_ sender: NSMenuItem) {
        if connect != 0 {
            var input : UInt64 = 0x84
            var outputSize = 2;
            var output : [UInt8] = Array(repeating: 0, count: 2)
            if kIOReturnSuccess == IOConnectCallMethod(connect, UInt32(kYSMCUCReadEC), &input, 1, nil, 0, nil, nil, &output, &outputSize),
               outputSize == 2 {
                let vFanSpeed = Int32(output[0]) | Int32(output[1]) << 8
                vFan.title = "Fan: \(vFanSpeed) rpm"
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
            updateFan(vFan)
        }
    }

    func applicationDidFinishLaunching(_ aNotification: Notification) {
        statusItem = NSStatusBar.system.statusItem(withLength: -1)

        guard let button = statusItem?.button else {
            print("status bar item failed. Try removing some menu bar item.")
            NSApp.terminate(nil)
            return
        }
        
        button.title = "⎇"
        button.target = self
        button.action = #selector(displayMenu)

        io_service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("YogaVPC"))
        if io_service != 0 {
            var CFProps : Unmanaged<CFMutableDictionary>? = nil
            if (kIOReturnSuccess == IORegistryEntryCreateCFProperties(io_service, &CFProps, kCFAllocatorDefault, 0) && CFProps != nil) {
                if let props = CFProps?.takeRetainedValue() as NSDictionary? {
                    let build = props["YogaSMC,Build"] as? NSString
                    vBuild.title = "Build: \(build ?? "Unknown")"
                    let version = props["YogaSMC,Version"] as? NSString
                    vVersion.title = "Version: \(version ?? "Unknown")"
                    switch props["IOClass"] as? NSString {
                    case "IdeaVPC":
                        registerNotification()
                        vClass.title = "Class: Idea"
                    case "ThinkVPC":
                        registerNotification()
                        vFan.isHidden = false
                        vClass.title = "Class: Think"
                    default:
                        vClass.title = "Class: Unknown"
                        break
                    }
                }
            }
        }
    }

    func applicationWillTerminate(_ aNotification: Notification) {
        // Insert code here to tear down your application
        if connect != 0 {
            IOServiceClose(connect);
        }
    }

    func registerNotification() {
        var notificationPort : CFMachPort?
        if kIOReturnSuccess == IOServiceOpen(io_service, mach_task_self_, 0, &connect),
           connect != 0 ,
           kIOReturnSuccess == IOConnectCallScalarMethod(connect, UInt32(kYSMCUCOpen), nil, 0, nil, nil) {
            var portContext = CFMachPortContext(version: 0, info: nil, retain: nil, release: nil, copyDescription: nil)
            notificationPort = CFMachPortCreate(kCFAllocatorDefault, notificationCallback, &portContext, nil)
            if notificationPort != nil  {
                let runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, notificationPort, 0);
                CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, .defaultMode);
            }
            IOConnectSetNotificationPort(connect, 0, CFMachPortGetPort(notificationPort), 0);
        }
    }

    // from https://github.com/MonitorControl/MonitorControl
    func setStartAtLogin(enabled: Bool) {
        let identifier = "\(Bundle.main.bundleIdentifier!)Helper" as CFString
        SMLoginItemSetEnabled(identifier, enabled)
        os_log("Toggle start at login state: %{public}@", type: .info, enabled ? "on" : "off")
    }
}

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

func OSD(_ prompt: String) {
    // from https://ffried.codes/2018/01/20/the-internals-of-the-macos-hud/
    let conn = NSXPCConnection(machServiceName: "com.apple.OSDUIHelper", options: [])
    conn.remoteObjectInterface = NSXPCInterface(with: OSDUIHelperProtocol.self)
    conn.interruptionHandler = { os_log("Interrupted!", type: .error) }
    conn.invalidationHandler = { os_log("Invalidated!", type: .error) }
    conn.resume()

    let target = conn.remoteObjectProxyWithErrorHandler { os_log("Failed: %@", type: .error, $0 as CVarArg) }
    guard let helper = target as? OSDUIHelperProtocol else { os_log("Wrong type %@", type: .fault, target as! CVarArg); return }


//    After move to /Applications
//    let path = "\(Bundle.main.resourcePath ?? "/Applications/YogaSMCNC.app/Contents/Resources")/kBrightOff.pdf"
    let path = "/System/Library/CoreServices/OSDUIHelper.app/Contents/Resources/kBrightOff.pdf"
    helper.showImageAtPath(path as NSString, onDisplayID: CGMainDisplayID(), priority: 0x1f4, msecUntilFade: 1000, withText: prompt as NSString)
}

func notificationCallback (_ port : CFMachPort?, _ msg : UnsafeMutableRawPointer?, _ size : CFIndex, _ info : UnsafeMutableRawPointer?) {
    if let notification = msg?.bindMemory(to: UInt32.self, capacity: 10){
        let arrPtr = UnsafeBufferPointer(start: notification, count: 10)
        let output = Array(arrPtr)
        let prompt = String(format:"Event %x", output[6])
        OSD(prompt)
    } else {
        OSD("Event unknown")
    }
}
