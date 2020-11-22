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

func bluetoothHelper(_ name: String) {
    guard IOBluetoothPreferencesAvailable() != 0 else {
        showOSDRes("Bluetooth", "Unavailable", .Bluetooth)
        os_log("Bluetooth unavailable!", type: .error)
        return
    }
    if IOBluetoothPreferenceGetControllerPowerState() != 0 {
        IOBluetoothPreferenceSetControllerPowerState(0)
        showOSDRes(name.isEmpty ? "Bluetooth" : name, "Off", .Bluetooth)
    } else {
        IOBluetoothPreferenceSetControllerPowerState(1)
        showOSDRes(name.isEmpty ? "Bluetooth" : name, "On", .Bluetooth)
    }
}

func bluetoothDiscoverableHelper(_ name: String) {
    guard IOBluetoothPreferencesAvailable() != 0 else {
        showOSDRes("Bluetooth", "Unavailable", .Bluetooth)
        os_log("Bluetooth unavailable!", type: .error)
        return
    }
    if IOBluetoothPreferenceGetDiscoverableState() != 0 {
        IOBluetoothPreferenceSetDiscoverableState(0)
        showOSDRes(name.isEmpty ? "BT Discoverable" : name, "Off", .Bluetooth)
    } else {
        IOBluetoothPreferenceSetDiscoverableState(1)
        showOSDRes(name.isEmpty ? "BT Discoverable" : name, "On", .Bluetooth)
    }
}

func wirelessHelper(_ name: String) {
    guard let iface = CWWiFiClient.shared().interface() else {
        showOSDRes("Wireless", "Unavailable", .Wifi)
        os_log("Wireless unavailable!", type: .error)
        return
    }
    let status = !iface.powerOn()
    do {
        try iface.setPower(status)
        if status {
            showOSDRes(name, "On", .Wifi)
        } else {
            showOSDRes(name, "Off", .WifiOff)
        }
    } catch {
        showOSDRes("Wireless", "Toggle failed", .Wifi)
        os_log("Wireless toggle failed!", type: .error)
    }
}

func airplaneModeHelper(_ name: String) {
    guard IOBluetoothPreferencesAvailable() != 0 else {
        showOSDRes("Bluetooth", "Unavailable", .Bluetooth)
        os_log("Bluetooth unavailable!", type: .error)
        return
    }
    guard let iface = CWWiFiClient.shared().interface() else {
        showOSDRes("Wireless", "Unavailable", .Wifi)
        os_log("Wireless unavailable!", type: .error)
        return
    }
    if IOBluetoothPreferenceGetControllerPowerState() == 0,
       !iface.powerOn() {
        IOBluetoothPreferenceSetControllerPowerState(1)
        do {
            try iface.setPower(true)
            showOSDRes(name, "Off", .Antenna)
        } catch {
            showOSDRes("Wireless", "Toggle failed", .Wifi)
            os_log("Wireless toggle failed!", type: .error)
        }
    } else {
        IOBluetoothPreferenceSetControllerPowerState(0)
        do {
            try iface.setPower(false)
            showOSDRes(name, "On", .AirplaneMode)
        } catch {
            showOSDRes("Wireless", "Toggle failed", .Wifi)
            os_log("Wireless toggle failed!", type: .error)
        }
    }
}
