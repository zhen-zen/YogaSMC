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

func bluetoothHelper() {
    guard IOBluetoothPreferencesAvailable() != 0 else {
        showOSDRes("Bluetooth Unavailable", .Bluetooth)
        os_log("Bluetooth unavailable!", type: .error)
        return
    }
    if IOBluetoothPreferenceGetControllerPowerState() != 0 {
        IOBluetoothPreferenceSetControllerPowerState(0)
        showOSDRes("Bluetooth Off", .Bluetooth)
    } else {
        IOBluetoothPreferenceSetControllerPowerState(1)
        showOSDRes("Bluetooth On", .Bluetooth)
    }
}

func bluetoothDiscoverableHelper() {
    guard IOBluetoothPreferencesAvailable() != 0 else {
        showOSDRes("Bluetooth Unavailable", .Bluetooth)
        os_log("Bluetooth unavailable!", type: .error)
        return
    }
    if IOBluetoothPreferenceGetDiscoverableState() != 0 {
        IOBluetoothPreferenceSetDiscoverableState(0)
        showOSDRes("BT Discoverable Off", .Bluetooth)
    } else {
        IOBluetoothPreferenceSetDiscoverableState(1)
        showOSDRes("BT Discoverable On", .Bluetooth)
    }
}

func wirelessHelper() {
    guard let iface = CWWiFiClient.shared().interface() else {
        showOSDRes("Wireless Unavailable", .Wifi)
        os_log("Wireless unavailable!", type: .error)
        return
    }
    do {
        try iface.setPower(!iface.powerOn())
        if iface.powerOn() {
            showOSDRes("Wireless On", .Wifi)
        } else {
            showOSDRes("Wireless Off", .WifiOff)
        }
    } catch {
        showOSDRes("Wireless Toggle failed", .Wifi)
        os_log("Wireless toggle failed!", type: .error)
    }
    os_log("%d", iface.interfaceMode().rawValue)
}

func airplaneModeHelper() {
    guard IOBluetoothPreferencesAvailable() != 0 else {
        showOSDRes("Bluetooth Unavailable", .Bluetooth)
        os_log("Bluetooth unavailable!", type: .error)
        return
    }
    guard let iface = CWWiFiClient.shared().interface() else {
        showOSDRes("Wireless Unavailable", .Wifi)
        os_log("Wireless unavailable!", type: .error)
        return
    }
    if IOBluetoothPreferenceGetControllerPowerState() == 0,
       !iface.powerOn() {
        IOBluetoothPreferenceSetControllerPowerState(1)
        do {
            try iface.setPower(true)
            showOSDRes("Airplane Mode Off", .Antenna)
        } catch {
            showOSDRes("Wireless Toggle failed", .Wifi)
            os_log("Wireless toggle failed!", type: .error)
        }
    } else {
        IOBluetoothPreferenceSetControllerPowerState(0)
        do {
            try iface.setPower(false)
            showOSDRes("Airplane Mode On", .AirplaneMode)
        } catch {
            showOSDRes("Wireless Toggle failed", .Wifi)
            os_log("Wireless toggle failed!", type: .error)
        }
    }
}
