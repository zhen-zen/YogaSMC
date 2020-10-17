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
        showOSD("Bluetooth Unavailable")
        os_log("Bluetooth unavailable!", type: .error)
        return
    }
    if IOBluetoothPreferenceGetControllerPowerState() != 0 {
        IOBluetoothPreferenceSetControllerPowerState(0)
        showOSD("Bluetooth Off")
    } else {
        IOBluetoothPreferenceSetControllerPowerState(1)
        showOSD("Bluetooth On")
    }
}

func bluetoothDiscoverableHelper() {
    guard IOBluetoothPreferencesAvailable() != 0 else {
        showOSD("Bluetooth Unavailable")
        os_log("Bluetooth unavailable!", type: .error)
        return
    }
    if IOBluetoothPreferenceGetDiscoverableState() != 0 {
        IOBluetoothPreferenceSetDiscoverableState(0)
        showOSD("BT Discoverable Off")
    } else {
        IOBluetoothPreferenceSetDiscoverableState(1)
        showOSD("BT Discoverable On")
    }
}

func wirelessHelper() {
    guard let iface = CWWiFiClient.shared().interface() else {
        showOSD("Wireless Unavailable")
        os_log("Wireless unavailable!", type: .error)
        return

    }
    do {
        try iface.setPower(!iface.powerOn())
        showOSD(iface.powerOn() ? "Wireless On" : "Wireless Off")
    } catch {
        showOSD("Wireless Toggle failed")
    }
    os_log("%d", iface.interfaceMode().rawValue)
}

func airplaneModeHelper() {
    guard IOBluetoothPreferencesAvailable() != 0 else {
        showOSD("Bluetooth Unavailable")
        os_log("Bluetooth unavailable!", type: .error)
        return
    }
    guard let iface = CWWiFiClient.shared().interface() else {
        showOSD("Wireless Unavailable")
        os_log("Wireless unavailable!", type: .error)
        return

    }
    if IOBluetoothPreferenceGetControllerPowerState() == 0,
       !iface.powerOn() {
        IOBluetoothPreferenceSetControllerPowerState(1)
        do {
            try iface.setPower(true)
            showOSD("Airplane Mode Off")
        } catch {
            showOSD("Wireless Toggle failed")
        }
    } else {
        IOBluetoothPreferenceSetControllerPowerState(0)
        do {
            try iface.setPower(false)
            showOSD("Airplane Mode On")
        } catch {
            showOSD("Wireless Toggle failed")
        }
    }
}
