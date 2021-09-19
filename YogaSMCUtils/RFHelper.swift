//
//  RFHelper.swift
//  YogaSMCNC
//
//  Created by Zhen on 10/16/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import Foundation
import IOBluetooth
import os.log
import CoreWLAN

func bluetoothHelper(_ name: String, _ display: Bool) {
    guard IOBluetoothPreferencesAvailable() != 0 else {
        showOSDRes("Bluetooth", "Unavailable", .kBluetooth)
        if #available(macOS 10.12, *) {
            os_log("Bluetooth unavailable!", type: .error)
        }
        return
    }
    let status = (IOBluetoothPreferenceGetControllerPowerState() == 0)
    IOBluetoothPreferenceSetControllerPowerState(status ? 1 : 0)
    if display {
        showOSDRes(name.isEmpty ? "Bluetooth" : name, status ? "On" : "Off", .kBluetooth)
    }
}

func bluetoothDiscoverableHelper(_ name: String, _ display: Bool) {
    guard IOBluetoothPreferencesAvailable() != 0 else {
        showOSDRes("Bluetooth", "Unavailable", .kBluetooth)
        if #available(macOS 10.12, *) {
            os_log("Bluetooth unavailable!", type: .error)
        }
        return
    }
    let status = (IOBluetoothPreferenceGetDiscoverableState() == 0)
    IOBluetoothPreferenceSetDiscoverableState(status ? 1 : 0)
    if display {
        showOSDRes(name.isEmpty ? "BT Discoverable" : name, status ? "On" : "Off", .kBluetooth)
    }
}

func wirelessHelper(_ name: String, _ display: Bool) {
    guard let iface = CWWiFiClient.shared().interface() else {
        showOSDRes("Wireless", "Unavailable", .kWifi)
        if #available(macOS 10.12, *) {
            os_log("Wireless unavailable!", type: .error)
        }
        return
    }
    let status = !iface.powerOn()
    do {
        try iface.setPower(status)
        if display {
            showOSDRes(name, status ? "On" : "Off", status ? .kWifi : .kWifiOff)
        }
    } catch {
        showOSDRes("Wireless", "Toggle failed", .kWifi)
        if #available(macOS 10.12, *) {
            os_log("Wireless toggle failed!", type: .error)
        }
    }
}

func airplaneModeHelper(_ name: String, _ display: Bool) {
    guard IOBluetoothPreferencesAvailable() != 0 else {
        showOSDRes("Bluetooth", "Unavailable", .kBluetooth)
        if #available(macOS 10.12, *) {
            os_log("Bluetooth unavailable!", type: .error)
        }
        return
    }
    guard let iface = CWWiFiClient.shared().interface() else {
        showOSDRes("Wireless", "Unavailable", .kWifi)
        if #available(macOS 10.12, *) {
            os_log("Wireless unavailable!", type: .error)
        }
        return
    }
    let status = (IOBluetoothPreferenceGetDiscoverableState() == 0 && !iface.powerOn())
    do {
        try iface.setPower(status)
        IOBluetoothPreferenceSetControllerPowerState(status ? 1 : 0)
        if display {
            showOSDRes(name, status ? "Off" : "On", status ? .kAntenna : .kAirplaneMode)
        }
    } catch {
        showOSDRes("Wireless", "Toggle failed", .kWifi)
        if #available(macOS 10.12, *) {
            os_log("Wireless toggle failed!", type: .error)
        }
    }
}
