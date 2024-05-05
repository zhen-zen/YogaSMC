//
//  YogaHIDD.cpp
//  YogaSMC
//
//  Created by Zhen on 11/4/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "YogaHIDD.hpp"
OSDefineMetaClassAndStructors(YogaHIDD, YogaVPC);

bool YogaHIDD::initDSM() {
    if (kIOReturnSuccess != vpc->validateObject("_DSM"))
        return false;

    OSObject *result = nullptr;
    if (kIOReturnSuccess != evaluateDSM(0, &result) || !result) {
        AlwaysLog("Failed to acquire fn mask");
        return false;
    }

    OSData *data = OSDynamicCast(OSData, result);
    if (!data) {
        AlwaysLog("Could not get valid return for fn mask");
        OSSafeReleaseNULL(result);
        return false;
    }

    switch (data->getLength()) {
        default:
            DebugLog("fn mask length = %d", data->getLength());
        case 2:
            fn_mask = *(reinterpret_cast<UInt16 const*>(data->getBytesNoCopy()));
            break;
        case 1:
            fn_mask = *(reinterpret_cast<UInt8 const*>(data->getBytesNoCopy()));
            break;
        case 0:
            AlwaysLog("Invalid length for fn mask");
            return false;
    }
    DebugLog("fn_mask = 0x%llx", fn_mask);
    OSSafeReleaseNULL(data);
    return true;
}

bool YogaHIDD::initVPC() {
    super::initVPC();

    if (!initDSM())
        AlwaysLog("Failed to obtain valid fn mask, evaluating methods directly");

    UInt64 mode = 0;
    if (kIOReturnSuccess != evaluateHIDD(INTEL_HID_DSM_HDMM_FN, &mode)) {
        AlwaysLog("Failed to read fn mode");
        return false;
    }
    // check simple mode
    if (mode != 0) {
        AlwaysLog("fn mode %llx not supported", mode);
        return false;
    }

    if (kIOReturnSuccess != evaluateHIDD(INTEL_HID_DSM_HDSM_FN, nullptr, true)) {
        AlwaysLog("Failed to enable hotkeys");
        return false;
    }

    UInt64 cap = 0;
    if ((kIOReturnSuccess == evaluateHIDD(INTEL_HID_DSM_HEBC_V2_FN, &cap) && (cap & 0x60000)) ||
        (kIOReturnSuccess == evaluateHIDD(INTEL_HID_DSM_HEBC_V1_FN, &cap) && (cap & 0x20000))) {
        DebugLog("5 button array or v2 power button is supported");
        initButtonArray();
    }
    setProperty("HEBC", cap, 32);
    return true;
}

bool YogaHIDD::exitVPC() {
    evaluateHIDD(INTEL_HID_DSM_HDSM_FN, nullptr, false);
    if (arrayCap != 0)
        evaluateHIDD(INTEL_HID_DSM_BTNE_FN, nullptr, 1);
    return super::exitVPC();
}

IOReturn YogaHIDD::message(UInt32 type, IOService *provider, void *argument) {
    if (type != kIOACPIMessageDeviceNotification || !argument)
        return super::message(type, provider, argument);

    UInt32 event = *(reinterpret_cast<UInt32 *>(argument));
    DebugLog("message: type=%x, provider=%s, argument=0x%04x", type, provider->getName(), event);
    switch (event) {
        case 0xc0: // HID events
            UInt64 index;
            if (kIOReturnSuccess != evaluateHIDD(INTEL_HID_DSM_HDEM_FN, &index)) {
                AlwaysLog("Failed to get event index");
                return kIOReturnSuccess;
            }
            switch (index) {
                case 3: // KEY_NUMLOCK
                    dispatchKeyEvent(ADB_NUM_LOCK, true);
                    dispatchKeyEvent(ADB_NUM_LOCK, false);
                    break;

                case 4: // KEY_HOME
                    dispatchKeyEvent(ADB_HOME, true);
                    dispatchKeyEvent(ADB_HOME, false);
                    break;

                case 5: // KEY_END
                    dispatchKeyEvent(ADB_END, true);
                    dispatchKeyEvent(ADB_END, false);
                    break;

                case 6: // KEY_PAGEUP
                    dispatchKeyEvent(ADB_PAGE_UP, true);
                    dispatchKeyEvent(ADB_PAGE_UP, false);
                    break;

                case 7: // KEY_PAGEDOWN
                    dispatchKeyEvent(ADB_PAGE_DOWN, true);
                    dispatchKeyEvent(ADB_PAGE_DOWN, false);
                    break;

                case 9: // KEY_POWER
                    dispatchKeyEvent(ADB_POWER, true);
                    dispatchKeyEvent(ADB_POWER, false);
                    break;

                case 15: // KEY_PLAYPAUSE
                    dispatchKeyEvent(ADB_PLAY_PAUSE, true);
                    dispatchKeyEvent(ADB_PLAY_PAUSE, false);
                    break;

                case 16: // KEY_MUTE
                    dispatchKeyEvent(ADB_MUTE, true);
                    dispatchKeyEvent(ADB_MUTE, false);
                    break;

                case 17: // KEY_VOLUMEUP
                    dispatchKeyEvent(ADB_VOLUME_UP, true);
                    dispatchKeyEvent(ADB_VOLUME_UP, false);
                    break;

                case 18: // KEY_VOLUMEDOWN
                    dispatchKeyEvent(ADB_VOLUME_DOWN, true);
                    dispatchKeyEvent(ADB_VOLUME_DOWN, false);
                    break;

                case 19: // KEY_BRIGHTNESSUP
                    dispatchKeyEvent(ADB_BRIGHTNESS_UP, true);
                    dispatchKeyEvent(ADB_BRIGHTNESS_UP, false);
                    break;

                case 20: // KEY_BRIGHTNESSDOWN
                    dispatchKeyEvent(ADB_BRIGHTNESS_DOWN, true);
                    dispatchKeyEvent(ADB_BRIGHTNESS_DOWN, false);
                    break;

                case 11: // KEY_SLEEP, notify SLPB?
                case 8: // KEY_RFKILL
                case 14: // KEY_STOPCD
                default:
                    if (client)
                        client->sendNotification(event);
                    break;
            }
            break;
        // Notify PWRB?
        case 0xce: // KEY_POWER DOWN
            dispatchKeyEvent(ADB_POWER, true);
            break;

        case 0xcf: // KEY_POWER UP
            dispatchKeyEvent(ADB_POWER, false);
            break;

        case 0xc2: // KEY_LEFTMETA DOWN
            dispatchKeyEvent(ADB_LEFT_META, true);
            break;

        case 0xc3: // KEY_LEFTMETA UP
            dispatchKeyEvent(ADB_LEFT_META, false);
            break;

        case 0xc4: // KEY_VOLUMEUP DOWN
            dispatchKeyEvent(ADB_VOLUME_UP, true);
            break;

        case 0xc5: // KEY_VOLUMEUP UP
            dispatchKeyEvent(ADB_VOLUME_UP, false);
            break;

        case 0xc6: // KEY_VOLUMEDOWN DOWN
            dispatchKeyEvent(ADB_VOLUME_DOWN, true);
            break;

        case 0xc7: // KEY_VOLUMEDOWN UP
            dispatchKeyEvent(ADB_VOLUME_DOWN, false);
            break;

        case 0xc8: // KEY_ROTATE_LOCK_TOGGLE DOWN
        case 0xc9: // KEY_ROTATE_LOCK_TOGGLE UP
        case 0xcc: // KEY_CONVERTIBLE_TOGGLE DOWN
        case 0xcd: // KEY_CONVERTIBLE_TOGGLE UP
        default:
            if (client)
                client->sendNotification(event);
            break;
    }
    return kIOReturnSuccess;
}

IOReturn YogaHIDD::setPowerState(unsigned long powerStateOrdinal, IOService * whatDevice) {
    if (super::setPowerState(powerStateOrdinal, whatDevice) != kIOPMAckImplied)
        return kIOReturnInvalid;

    if (powerStateOrdinal == 0) {
        // Actually unnecessary for s2idle
        evaluateHIDD(INTEL_HID_DSM_HDSM_FN, nullptr, false);
        if (arrayCap != 0)
            evaluateHIDD(INTEL_HID_DSM_BTNE_FN, nullptr, 1);
    } else {
        evaluateHIDD(INTEL_HID_DSM_HDSM_FN, nullptr, true);
        if (arrayCap != 0)
            evaluateHIDD(INTEL_HID_DSM_BTNE_FN, nullptr, arrayCap);
    }

    return kIOPMAckImplied;
}

IOReturn YogaHIDD::evaluateDSM(UInt32 index, OSObject **result, OSArray *arg, const char *uuid, UInt32 revision) {
    IOReturn ret;
    uuid_t guid;
    uuid_parse(uuid, guid);

    // convert to mixed-endian
    *(reinterpret_cast<uint32_t *>(guid)) = OSSwapInt32(*(reinterpret_cast<uint32_t *>(guid)));
    *(reinterpret_cast<uint16_t *>(guid) + 2) = OSSwapInt16(*(reinterpret_cast<uint16_t *>(guid) + 2));
    *(reinterpret_cast<uint16_t *>(guid) + 3) = OSSwapInt16(*(reinterpret_cast<uint16_t *>(guid) + 3));

    OSObject *params[] = {
        OSData::withBytes(guid, 16),
        OSNumber::withNumber(revision, 32),
        OSNumber::withNumber(index, 32),
        arg ? arg : OSArray::withCapacity(1),
    };

    ret = vpc->evaluateObject("_DSM", result, params, 4);

    params[0]->release();
    params[1]->release();
    params[2]->release();
    if (!arg)
        params[3]->release();
    return ret;
}

IOReturn YogaHIDD::evaluateHIDD(intel_hid_dsm_fn_codes index, UInt64 *value, SInt64 arg) {
    IOReturn ret;
    if (index <= INTEL_HID_DSM_FN_INVALID ||
        index >= INTEL_HID_DSM_FN_MAX)
        return kIOReturnUnsupported;

    OSObject *obj = nullptr;
    if (fn_mask & BIT(index)) {
        OSArray *arr = nullptr;
        if (arg >= 0) {
            arr = OSArray::withCapacity(1);
            OSNumber *val = OSNumber::withNumber(arg, 32);
            arr->setObject(val);
            OSSafeReleaseNULL(val);
        }
        ret = evaluateDSM(index, &obj, arr);
        OSSafeReleaseNULL(arr);
    } else {
        if (arg >= 0) {
            OSObject *params[] = {
                OSNumber::withNumber(arg, 32),
            };
            ret = vpc->evaluateObject(intel_hid_dsm_fn_to_method[index], &obj, params, 1);
            params[0]->release();
        } else {
            ret = vpc->evaluateObject(intel_hid_dsm_fn_to_method[index], &obj);
        }
    }

    if (ret == kIOReturnSuccess && value) {
        OSNumber *number = OSDynamicCast(OSNumber, obj);
        if (number != nullptr) {
            *value = number->unsigned64BitValue();
            DebugLog("index %d results 0x%llx", index, *value);
        } else {
            AlwaysLog("index %d results invalid", index);
            ret = kIOReturnInvalid;
        }
    }
    OSSafeReleaseNULL(obj);
    return ret;
}

void YogaHIDD::initButtonArray() {
    if (kIOReturnSuccess != vpc->evaluateInteger("BTNC", &arrayCap)) {
        AlwaysLog("Failed to get button capability");
        return;
    }

    setProperty("Array Button", arrayCap, 32);
    if (arrayCap == 0)
        DebugLog("No available button capability");

    if (kIOReturnSuccess != evaluateHIDD(INTEL_HID_DSM_BTNE_FN, nullptr, arrayCap))
        AlwaysLog("Failed to set button capability");
    else
        DebugLog("Set button capability success");

    if (kIOReturnSuccess != evaluateHIDD(INTEL_HID_DSM_BTNL_FN, nullptr, 0))
        AlwaysLog("Failed to enable HID power button");
}
