//
//  SystemPreferences.swift
//  YogaSMC
//
//  Created by Zhen on 10/15/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import ScriptingBridge

@objc public protocol SBObjectProtocol: NSObjectProtocol {
    func get() -> Any!
}

@objc public protocol SBApplicationProtocol: SBObjectProtocol {
    func activate()
    var delegate: SBApplicationDelegate! { get set }
    var isRunning: Bool { get }
}

// MARK: SystemPreferencesSaveOptions
@objc public enum SystemPreferencesSaveOptions: AEKeyword {
    case yes = 0x79657320 /* 'yes ' */
    case no = 0x6e6f2020 /* 'no  ' */
    case ask = 0x61736b20 /* 'ask ' */
}

// MARK: SystemPreferencesPrintingErrorHandling
@objc public enum SystemPreferencesPrintingErrorHandling: AEKeyword {
    case standard = 0x6c777374 /* 'lwst' */
    case detailed = 0x6c776474 /* 'lwdt' */
}

// MARK: System
@objc public protocol System {
    @objc optional func closeSaving(_ saving: SystemPreferencesSaveOptions, savingIn: Any!) // Close a document.
    @objc optional func printWithProperties(_ withProperties: Any!, printDialog: Any!) // Print a document.
}

// MARK: SystemPreferencesApplication
@objc public protocol SystemPreferencesApplication: SBApplicationProtocol {
    @objc optional func documents()
    @objc optional func windows() -> SBElementArray
    @objc optional var name: Int { get } // The name of the application.
    @objc optional var frontmost: Int { get } // Is this the active application?
    @objc optional var version: Int { get } // The version number of the application.
    @objc optional func `open`(_ x: Any!) -> Any // Open a document.
    @objc optional func print(_ x: Any!, withProperties: Any!, printDialog: Any!) // Print a document.
    @objc optional func quitSaving(_ saving: SystemPreferencesSaveOptions) // Quit the application.
    @objc optional func exists(_ x: Any!) // Verify that an object exists.
    @objc optional func panes() -> SBElementArray
    @objc optional var currentPane: SystemPreferencesPane { get } // the currently selected pane
    @objc optional var preferencesWindow: SystemPreferencesWindow { get } // the main preferences window
    @objc optional var showAll: Int { get } // Is SystemPrefs in show all view. (Setting to false will do nothing)
    @objc optional func setCurrentPane(_ currentPane: SystemPreferencesPane!) // the currently selected pane
    @objc optional func setShowAll(_ showAll: Int) // Is SystemPrefs in show all view. (Setting to false will do nothing)
}
extension SBApplication: SystemPreferencesApplication {}

// MARK: SystemPreferencesDocument
@objc public protocol SystemPreferencesDocument: SBObjectProtocol {
    @objc optional var modified: Int { get } // Has it been modified since the last save?
    @objc optional var file: Int { get } // Its location on disk, if it has one.
}
extension SBObject: SystemPreferencesDocument {}

// MARK: SystemPreferencesWindow
@objc public protocol SystemPreferencesWindow: SBObjectProtocol {
    @objc optional var name: String { get } // The title of the window
    @objc optional func id() // The unique identifier of the window.
    @objc optional var index: Int { get } // The index of the window, ordered front to back.
    @objc optional var bounds: Int { get } // The bounding rectangle of the window.
    @objc optional var closeable: Int { get } // Does the window have a close button?
    @objc optional var miniaturizable: Int { get } // Does the window have a minimize button?
    @objc optional var miniaturized: Int { get } // Is the window minimized right now?
    @objc optional var resizable: Int { get } // Can the window be resized?
    @objc optional var visible: Int { get } // Is the window visible right now?
    @objc optional var zoomable: Int { get } // Does the window have a zoom button?
    @objc optional var zoomed: Int { get } // Is the window zoomed right now?
    @objc optional var document: SystemPreferencesDocument { get } // The document whose contents are displayed in the window.
    @objc optional func setIndex(_ index: Int) // The index of the window, ordered front to back.
    @objc optional func setBounds(_ bounds: Int) // The bounding rectangle of the window.
    @objc optional func setMiniaturized(_ miniaturized: Int) // Is the window minimized right now?
    @objc optional func setVisible(_ visible: Int) // Is the window visible right now?
    @objc optional func setZoomed(_ zoomed: Int) // Is the window zoomed right now?
}
extension SBObject: SystemPreferencesWindow {}

// MARK: SystemPreferencesPane
@objc public protocol SystemPreferencesPane: SBObjectProtocol {
    @objc optional func anchors() -> SBElementArray
    @objc optional func id() -> String // locale independent name of the preference pane; can refer to a pane using the expression: pane id "<name>"
    @objc optional var localizedName: Int { get } // localized name of the preference pane
    @objc optional var name: String { get } // name of the preference pane as it appears in the title bar; can refer to a pane using the expression: pane "<name>"
    @objc optional func reveal() -> Any // Reveals an anchor within a preference pane or preference pane itself
    @objc optional func authorize() -> SystemPreferencesPane // Prompt authorization for given preference pane
}
extension SBObject: SystemPreferencesPane {}

// MARK: SystemPreferencesAnchor
@objc public protocol SystemPreferencesAnchor: SBObjectProtocol {
    @objc optional var name: String { get } // name of the anchor within a preference pane
    @objc optional func reveal() -> Any // Reveals an anchor within a preference pane or preference pane itself
}
extension SBObject: SystemPreferencesAnchor {}
