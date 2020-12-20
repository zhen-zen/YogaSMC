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
    mElement: kAudioObjectPropertyElementMaster)

let defaultOutputDeviceProp = AudioObjectPropertyAddress(
    mSelector: kAudioHardwarePropertyDefaultOutputDevice,
    mScope: kAudioObjectPropertyScopeGlobal,
    mElement: kAudioObjectPropertyElementMaster)

let masterInputVolumeProp = AudioObjectPropertyAddress(
    mSelector: kAudioHardwareServiceDeviceProperty_VirtualMasterVolume,
    mScope: kAudioDevicePropertyScopeInput,
    mElement: kAudioObjectPropertyElementMaster)

let masterOutputVolumeProp = AudioObjectPropertyAddress(
    mSelector: kAudioHardwareServiceDeviceProperty_VirtualMasterVolume,
    mScope: kAudioDevicePropertyScopeOutput,
    mElement: kAudioObjectPropertyElementMaster)

let muteInputVolumeProp = AudioObjectPropertyAddress(
    mSelector: kAudioDevicePropertyMute,
    mScope: kAudioDevicePropertyScopeInput,
    mElement: kAudioObjectPropertyElementMaster)

let muteOutputVolumeProp = AudioObjectPropertyAddress(
    mSelector: kAudioDevicePropertyMute,
    mScope: kAudioDevicePropertyScopeOutput,
    mElement: kAudioObjectPropertyElementMaster)

enum AudioPropertyError : Error {
    case NoProperty
    case PropertyNotSettable
    case GetPropertyData(OSStatus)
    case SetPropertyData(OSStatus)
    case EmptyResult
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
        var p = prop
        if (!AudioObjectHasProperty(device, &p)) {
            return .failure(.NoProperty)
        }

        var size = UInt32(MemoryLayout<T>.size)
        var result: T? = emptyValue
        let error = AudioObjectGetPropertyData(device, &p, 0, nil, &size, &result)

        if (error != noErr) {
            return .failure(.GetPropertyData(error))
        }

        if let result = result {
            return .success(result)
        }

        return .failure(.EmptyResult)
    }

    func set(_ newValue: T) -> Result<T, AudioPropertyError> {
        var p = prop
        if (!AudioObjectHasProperty(device, &p)) {
            return .failure(.NoProperty)
        }

        var settable: DarwinBoolean = false
        var error = AudioObjectIsPropertySettable(device, &p, &settable)
        if (error != noErr) {
            return .failure(.PropertyNotSettable)
        }

        let size = UInt32(MemoryLayout<T>.size)
        var value = newValue
        error = AudioObjectSetPropertyData(device, &p, 0, nil, size, &value)
        if (error != noErr) {
            return .failure(.SetPropertyData(error))
        }

        return .success(value)
    }
}

let inputDevice = try! AudioProperty<AudioDeviceID>(device: nil, emptyValue: nil, prop: defaultInputDeviceProp).get().get()
let outputDevice = try! AudioProperty<AudioDeviceID>(device: nil, emptyValue: nil, prop: defaultOutputDeviceProp).get().get()

let inputVolume = AudioProperty<Float>(device: inputDevice, emptyValue: 0, prop: masterInputVolumeProp)
let outputVolume = AudioProperty<Float>(device: outputDevice, emptyValue: 0, prop: masterOutputVolumeProp)

let inputMute = AudioProperty<UInt32>(device: inputDevice, emptyValue: 0, prop: muteInputVolumeProp)
let outputMute = AudioProperty<UInt32>(device: outputDevice, emptyValue: 0, prop: muteOutputVolumeProp)

func micMuteHelper(_ service: io_service_t, _ name: String) {
    do {
        if try inputMute.get().get() != 0 {
            if try inputVolume.get().get() == 0 {
                guard try inputVolume.set(0.50).get() == 0.50 else {
                    throw AudioPropertyError.EmptyResult
                }
            }
            guard try inputMute.set(0).get() == 0,
                  sendNumber("MicMuteLED", 0, service) else {
                throw AudioPropertyError.NoProperty
            }
            showOSDRes(name, "Unmute", .kMic)
        } else {
            guard try inputMute.set(1).get() == 1,
                  sendNumber("MicMuteLED", 2, service) else {
                throw AudioPropertyError.NoProperty
            }
            showOSDRes(name, "Mute", .kMicOff)
        }
    } catch {
        showOSDRes("Mic", "Toggle failed", .kMic)
        os_log("Mic toggle failed!", type: .error)
    }
}

func micMuteLEDHelper(_ service: io_service_t) {
    do {
        let mute = try inputMute.get().get() != 0
        guard sendNumber("MicMuteLED", mute ? 2 : 0, service) else {
            throw AudioPropertyError.NoProperty
        }
        os_log("Mic Mute LED updated", type: .info)
    } catch {
        os_log("Failed to update Mic Mute LED", type: .error)
    }
}

var muteLEDStatus = false

func muteLEDHelper(_ service: io_service_t, _ wake: Bool = true) {
    do {
        let mute = try outputMute.get().get() != 0
        if !wake, mute == muteLEDStatus {
            return
        }
        guard sendBoolean("MuteLED", mute, service) else {
            throw AudioPropertyError.NoProperty
        }
        muteLEDStatus = mute
        os_log("Mute LED updated", type: .info)
    } catch {
        os_log("Failed to update Mute LED", type: .error)
    }
}

class VolumeObserver: NSApplication {
    static let volumeChanged = Notification.Name("YogaSMCNC.volumeChanged")

    //from https://stackoverflow.com/a/32769093/6884062
    override func sendEvent(_ event: NSEvent) {
        if (event.type == .systemDefined && event.subtype.rawValue == 8) {
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
        switch(key) {
        case NX_KEYTYPE_SOUND_DOWN, NX_KEYTYPE_SOUND_UP, NX_KEYTYPE_MUTE:
            NotificationCenter.default.post(name: VolumeObserver.volumeChanged, object: self)
        default:
            break
        }
    }
}
