//
//  YogaSMCNCApp.swift
//  YogaSMCNC
//
//  Created by Zhen on 10/11/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import SwiftUI
import os.log

@main
struct YogaSMCNCApp: App {
    @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate
    var body: some Scene {
        WindowGroup {
            EmptyView()
        }
    }
}

// based on https://www.anaghsharma.com/blog/macos-menu-bar-app-with-swiftui/ and
// https://github.com/zaferarican/menubarpopoverswiftui2

class AppDelegate: NSObject, NSApplicationDelegate {
    var popover = NSPopover.init()
    var statusBarItem: NSStatusItem?
    var io_service : io_service_t = 0
    var version : String?

    func applicationDidFinishLaunching(_ notification: Notification) {
        io_service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("YogaVPC"))
        if io_service != 0 {
            var CFProps : Unmanaged<CFMutableDictionary>? = nil
            if (kIOReturnSuccess == IORegistryEntryCreateCFProperties(io_service, &CFProps, kCFAllocatorDefault, 0) && CFProps != nil) {
                if let props = CFProps?.takeRetainedValue() as NSDictionary? {
                    if let val = props["YogaSMC,Build"] as? NSString {
                        version = val as String
                    }
                    switch props["IOClass"] as? NSString {
                    case "IdeaVPC":
                        registerNotification()
                    case "ThinkVPC":
                        registerNotification()
//                    case "YogaVPC":
                    default:
                        break
                    }
                }
            }
        }
        let contentView = ContentView(version: version ?? "?")

        // Set the SwiftUI's ContentView to the Popover's ContentViewController
        popover.behavior = .transient // !!! - This does not seem to work in SwiftUI2.0 or macOS BigSur yet
        popover.animates = false
        popover.contentViewController = NSViewController()
        popover.contentViewController?.view = NSHostingView(rootView: contentView)
        statusBarItem = NSStatusBar.system.statusItem(withLength: NSStatusItem.variableLength)
        statusBarItem?.button?.image = NSImage.init(systemSymbolName: "rectangle", accessibilityDescription: "YogaSMCNC")
        statusBarItem?.button?.image?.isTemplate = true
        statusBarItem?.button?.action = #selector(AppDelegate.togglePopover(_:))
    }
    @objc func showPopover(_ sender: AnyObject?) {
        if let button = statusBarItem?.button {
            popover.show(relativeTo: button.bounds, of: button, preferredEdge: NSRectEdge.maxY)
            //            !!! - displays the popover window with an offset in x in macOS BigSur.
        }
    }
    @objc func closePopover(_ sender: AnyObject?) {
        popover.performClose(sender)
    }
    @objc func togglePopover(_ sender: AnyObject?) {
        if popover.isShown {
            closePopover(sender)
        } else {
            showPopover(sender)
        }
    }
    func registerNotification() {
        var connect : io_connect_t = 0;
        var notificationPort : CFMachPort?
        if kIOReturnSuccess == IOServiceOpen(io_service, mach_task_self_, 0, &connect),
           connect != 0{
            if kIOReturnSuccess == IOConnectCallScalarMethod(connect, UInt32(kYSMCUCOpen), nil, 0, nil, nil) {
                var portContext = CFMachPortContext(version: 0, info: nil, retain: nil, release: nil, copyDescription: nil)
                notificationPort = CFMachPortCreate(kCFAllocatorDefault, notificationCallback, &portContext, nil)
                if notificationPort != nil  {
                    let runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, notificationPort, 0);
                    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, .defaultMode);
                }
                IOConnectSetNotificationPort(connect, 0, CFMachPortGetPort(notificationPort), 0);
                //                IOServiceClose(connect)
            }
        }
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
    conn.interruptionHandler = { os_log("Interrupted!", type: .debug) }
    conn.invalidationHandler = { os_log("Invalidated!", type: .error) }
    conn.resume()

    let target = conn.remoteObjectProxyWithErrorHandler { os_log("Failed: %@", type: .error, $0 as CVarArg) }
    guard let helper = target as? OSDUIHelperProtocol else { os_log("Wrong type %@", type: .fault, target as! CVarArg); return }

    helper.showImageAtPath("/System/Library/CoreServices/OSDUIHelper.app/Contents/Resources/kBright.pdf", onDisplayID: CGMainDisplayID(), priority: 0x1f4, msecUntilFade: 2000, withText: prompt as NSString)
}

func notificationCallback (_ port : CFMachPort?, _ msg : UnsafeMutableRawPointer?, _ size : CFIndex, _ info : UnsafeMutableRawPointer?) {
    if let notification = msg?.load(as: SMCNotificationMessage.self) {
        let prompt = String(format:"Event %x", notification.event)
        OSD(prompt)
    } else {
        OSD("Event unknown")
    }
}
