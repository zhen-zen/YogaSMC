//
//  AudioHelper.swift
//  YogaSMCNC
//
//  Created by Zhen on 12/9/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

import Foundation
import CoreAudio
import AudioToolbox
import AppKit
import os.log

// from https://gist.github.com/mblsha/f6db715ca6245b36eb1fa6d8af23cf4b

let defaultInputDeviceProp = AudioObjectPropertyAddress(
    mSelector: kAudioHardwarePropertyDefaultInputDevice,
    mScope: kAudioObjectPropertyScopeGlobal,
    mElement: kAudioObjectPropertyElementMain)

let defaultOutputDeviceProp = AudioObjectPropertyAddress(
    mSelector: kAudioHardwarePropertyDefaultOutputDevice,
    mScope: kAudioObjectPropertyScopeGlobal,
    mElement: kAudioObjectPropertyElementMain)

let primaryInputVolumeProp = AudioObjectPropertyAddress(
    mSelector: kAudioHardwareServiceDeviceProperty_VirtualMainVolume,
    mScope: kAudioDevicePropertyScopeInput,
    mElement: kAudioObjectPropertyElementMain)

let primaryOutputVolumeProp = AudioObjectPropertyAddress(
    mSelector: kAudioHardwareServiceDeviceProperty_VirtualMainVolume,
    mScope: kAudioDevicePropertyScopeOutput,
    mElement: kAudioObjectPropertyElementMain)

let muteInputVolumeProp = AudioObjectPropertyAddress(
    mSelector: kAudioDevicePropertyMute,
    mScope: kAudioDevicePropertyScopeInput,
    mElement: kAudioObjectPropertyElementMain)

let muteOutputVolumeProp = AudioObjectPropertyAddress(
    mSelector: kAudioDevicePropertyMute,
    mScope: kAudioDevicePropertyScopeOutput,
    mElement: kAudioObjectPropertyElementMain)

enum AudioPropertyError: Error {
    case noProperty
    case propertyNotSettable
    case getPropertyData(OSStatus)
    case setPropertyData(OSStatus)
    case emptyResult
}

struct AudioProperty<T> {
    let emptyValue: T?
    let device: AudioDeviceID
    let prop: AudioObjectPropertyAddress

    // For numerical values if we don't initialize 'result: T?' to non-nil
    // value we'll get .EmptyResult error if the value is zero.
    init(device: AudioDeviceID?, emptyValue: T?, prop: AudioObjectPropertyAddress) {
        self.emptyValue = emptyValue
        self.device = device ?? UInt32(kAudioObjectSystemObject)
        self.prop = prop
    }

    func get() -> Result<T, AudioPropertyError> {
        var addr = prop
        if !AudioObjectHasProperty(device, &addr) {
            return .failure(.noProperty)
        }

        var size = UInt32(MemoryLayout<T>.size)
        var result: T? = emptyValue
        let error = AudioObjectGetPropertyData(device, &addr, 0, nil, &size, &result)

        if error != noErr {
            return .failure(.getPropertyData(error))
        }

        if let result = result {
            return .success(result)
        }

        return .failure(.emptyResult)
    }

    func set(_ newValue: T) -> Result<T, AudioPropertyError> {
        var addr = prop
        if !AudioObjectHasProperty(device, &addr) {
            return .failure(.noProperty)
        }

        var settable: DarwinBoolean = false
        var error = AudioObjectIsPropertySettable(device, &addr, &settable)
        if error != noErr {
            return .failure(.propertyNotSettable)
        }

        let size = UInt32(MemoryLayout<T>.size)
        var value = newValue
        error = AudioObjectSetPropertyData(device, &addr, 0, nil, size, &value)
        if error != noErr {
            return .failure(.setPropertyData(error))
        }

        return .success(value)
    }
}

class AudioHelper {
    let inputDevice: AudioDeviceID
    let outputDevice: AudioDeviceID

    let inputVolume: AudioProperty<Float>
    let outputVolume: AudioProperty<Float>

    let inputMute: AudioProperty<UInt32>
    let outputMute: AudioProperty<UInt32>

    var muteLEDStatus = false

    static let shared = AudioHelper()

    init?() {
        do {
            try inputDevice = AudioProperty<AudioDeviceID>(
                device: nil,
                emptyValue: nil,
                prop: defaultInputDeviceProp
            ).get().get()
            try outputDevice = AudioProperty<AudioDeviceID>(
                device: nil,
                emptyValue: nil,
                prop: defaultOutputDeviceProp
            ).get().get()
        } catch {
            if #available(macOS 10.12, *) {
                os_log("Failed to initialize AudioHelper!", type: .error)
            }
            return nil
        }

        self.inputVolume = AudioProperty<Float>(device: inputDevice, emptyValue: 0, prop: primaryInputVolumeProp)
        self.outputVolume = AudioProperty<Float>(device: outputDevice, emptyValue: 0, prop: primaryOutputVolumeProp)

        self.inputMute = AudioProperty<UInt32>(device: inputDevice, emptyValue: 0, prop: muteInputVolumeProp)
        self.outputMute = AudioProperty<UInt32>(device: outputDevice, emptyValue: 0, prop: muteOutputVolumeProp)
    }

    func micMuteHelper(_ service: io_service_t, _ name: String) {
        do {
            if try inputMute.get().get() != 0 {
                if try inputVolume.get().get() == 0 {
                    guard try inputVolume.set(0.50).get() == 0.50 else {
                        throw AudioPropertyError.emptyResult
                    }
                }
                guard try inputMute.set(0).get() == 0,
                      sendNumber("MicMuteLED", 0, service) else {
                    throw AudioPropertyError.noProperty
                }
                showOSDRes(name, "Unmute", .kMic)
            } else {
                guard try inputMute.set(1).get() == 1,
                      sendNumber("MicMuteLED", 2, service) else {
                    throw AudioPropertyError.noProperty
                }
                showOSDRes(name, "Mute", .kMicOff)
            }
        } catch {
            showOSDRes("Mic", "Toggle failed", .kMic)
            if #available(macOS 10.12, *) {
                os_log("Mic toggle failed!", type: .error)
            }
        }
    }

    func micMuteLEDHelper(_ service: io_service_t) {
        do {
            let mute = try inputMute.get().get() != 0
            guard sendNumber("MicMuteLED", mute ? 2 : 0, service) else {
                throw AudioPropertyError.noProperty
            }
            if #available(macOS 10.12, *) {
                os_log("Mic Mute LED updated", type: .info)
            }
        } catch {
            if #available(macOS 10.12, *) {
                os_log("Failed to update Mic Mute LED", type: .error)
            }
        }
    }

    func muteLEDHelper(_ service: io_service_t, _ wake: Bool) {
        do {
            let mute = try outputMute.get().get() != 0
            if !wake, mute == muteLEDStatus { return }
            guard sendBoolean("MuteLED", mute, service) else {
                throw AudioPropertyError.noProperty
            }
            muteLEDStatus = mute
            if #available(macOS 10.12, *) {
                os_log("Mute LED updated", type: .info)
            }
        } catch {
            if #available(macOS 10.12, *) {
                os_log("Failed to update Mute LED", type: .error)
            }
        }
    }
}

class VolumeObserver: NSApplication {
    static let volumeChanged = Notification.Name("YogaSMCNC.volumeChanged")

    //from https://stackoverflow.com/a/32769093/6884062
    override func sendEvent(_ event: NSEvent) {
        if event.type == .systemDefined && event.subtype.rawValue == 8 {
            let keyCode = ((event.data1 & 0xFFFF0000) >> 16)
            let keyFlags = (event.data1 & 0x0000FFFF)
            // Get the key state. 0xA is KeyDown, OxB is KeyUp
            let keyState = (((keyFlags & 0xFF00) >> 8)) == 0xA
            let keyRepeat = keyFlags & 0x1
            mediaKeyEvent(key: Int32(keyCode), state: keyState, keyRepeat: Bool(truncating: keyRepeat as NSNumber))
        }

        super.sendEvent(event)
    }

    func mediaKeyEvent(key: Int32, state: Bool, keyRepeat: Bool) {
        switch key {
        case NX_KEYTYPE_SOUND_DOWN, NX_KEYTYPE_SOUND_UP, NX_KEYTYPE_MUTE:
            NotificationCenter.default.post(name: VolumeObserver.volumeChanged, object: self)
        default:
            break
        }
    }
}

func micMuteLEDHelper(_ service: io_service_t) {
    if AudioHelper.shared != nil {
        AudioHelper.shared?.micMuteLEDHelper(service)
    }

    guard let current = scriptHelper(getMicVolumeAS, "MicMute") else { return }
    _ = sendNumber("MicMuteLED", current.int32Value != 0 ? 0 : 2, service)
}

var muteLEDStatus = false

func muteLEDHelper(_ service: io_service_t, _ wake: Bool = true) {
    if AudioHelper.shared != nil {
        AudioHelper.shared?.muteLEDHelper(service, wake)
        return
    }

    guard let current = scriptHelper(getAudioMutedAS, "Mute") else { return }
    if !wake, current.booleanValue == muteLEDStatus { return }
    guard sendBoolean("MuteLED", current.booleanValue, service) else {
        if #available(macOS 10.12, *) {
            os_log("Failed to update Mute LED", type: .error)
        }
        return
    }
    muteLEDStatus = current.booleanValue
    if #available(macOS 10.12, *) {
        os_log("Mute LED updated", type: .info)
    }
}

var muteVolume: Int32 = 50

func micMuteHelper(_ service: io_service_t, _ name: String) {
    if AudioHelper.shared != nil {
        AudioHelper.shared?.micMuteHelper(service, name)
        return
    }

    guard let current = scriptHelper(getMicVolumeAS, "MicMute") else { return }
    if current.int32Value != 0 {
        if scriptHelper(String(format: setMicVolumeAS, 0), "MicMute") != nil {
            muteVolume = current.int32Value
            _ = sendNumber("MicMuteLED", 2, service)
            showOSDRes(name, "Mute", .kMicOff)
        }
    } else {
        if scriptHelper(String(format: setMicVolumeAS, muteVolume), "MicMute") != nil {
            muteVolume = current.int32Value
            _ = sendNumber("MicMuteLED", 0, service)
            showOSDRes(name, "Unmute", .kMic)
        }
    }
}
