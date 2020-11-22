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

func bluetoothHelper(_ hideText: Bool) {
    guard IOBluetoothPreferencesAvailable() != 0 else {
        showOSDRes("BluetoothVar", "Unavailable", .Bluetooth)
        os_log("Bluetooth unavailable!", type: .error)
        return
    }
    if IOBluetoothPreferenceGetControllerPowerState() != 0 {
        IOBluetoothPreferenceSetControllerPowerState(0)
        showOSDRes("BluetoothVar", "Off", .Bluetooth)
    } else {
        IOBluetoothPreferenceSetControllerPowerState(1)
        showOSDRes("BluetoothVar", "On", .Bluetooth)
    }
}

func bluetoothDiscoverableHelper(_ hideText: Bool) {
    guard IOBluetoothPreferencesAvailable() != 0 else {
        showOSDRes("BluetoothVar", "Unavailable", .Bluetooth)
        os_log("Bluetooth unavailable!", type: .error)
        return
    }
    if IOBluetoothPreferenceGetDiscoverableState() != 0 {
        IOBluetoothPreferenceSetDiscoverableState(0)
        showOSDRes("BTDiscoverableVar", "Off", .Bluetooth)
    } else {
        IOBluetoothPreferenceSetDiscoverableState(1)
        showOSDRes("BTDiscoverableVar", "On", .Bluetooth)
    }
}

func wirelessHelper(_ hideText: Bool) {
    guard let iface = CWWiFiClient.shared().interface() else {
        showOSDRes("WirelessVar", "Unavailable", .Wifi)
        os_log("Wireless unavailable!", type: .error)
        return
    }
    let status = !iface.powerOn()
    do {
        try iface.setPower(status)
        if status {
            showOSDRes("WirelessVar", "On", .Wifi)
        } else {
            showOSDRes("WirelessVar", "Off", .WifiOff)
        }
    } catch {
        showOSDRes("WirelessVar", "Toggle failed", .Wifi)
        os_log("Wireless toggle failed!", type: .error)
    }
}

func airplaneModeHelper(_ hideText: Bool) {
    guard IOBluetoothPreferencesAvailable() != 0 else {
        showOSDRes("BluetoothVar", "Unavailable", .Bluetooth)
        os_log("Bluetooth unavailable!", type: .error)
        return
    }
    guard let iface = CWWiFiClient.shared().interface() else {
        showOSDRes("WirelessVar", "Unavailable", .Wifi)
        os_log("Wireless unavailable!", type: .error)
        return
    }
    if IOBluetoothPreferenceGetControllerPowerState() == 0,
       !iface.powerOn() {
        IOBluetoothPreferenceSetControllerPowerState(1)
        do {
            try iface.setPower(true)
            showOSDRes("AirplaneModeVar", "Off", .Antenna)
        } catch {
            showOSDRes("WirelessVar", "Toggle failed", .Wifi)
            os_log("Wireless toggle failed!", type: .error)
        }
    } else {
        IOBluetoothPreferenceSetControllerPowerState(0)
        do {
            try iface.setPower(false)
            showOSDRes("AirplaneModeVar", "On", .AirplaneMode)
        } catch {
            showOSDRes("WirelessVar", "Toggle failed", .Wifi)
            os_log("Wireless toggle failed!", type: .error)
        }
    }
}
