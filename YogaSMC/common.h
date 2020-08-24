//  SPDX-License-Identifier: GPL-2.0-only
//
//  common.h
//  YogaSMC
//
//  Created by Zhen on 8/23/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef common_h
#define common_h

#ifdef DEBUG
#define DebugLog(str, ...) do { IOLog("%s::%s " str, getName(), name, ## __VA_ARGS__); } while (0)
#else
#define DebugLog(str, ...) do { } while (0)
#endif
#define AlwaysLog(str, ...) do { IOLog("%s::%s " str, getName(), name, ## __VA_ARGS__); } while (0)

#define BIT(nr) (1U << (nr))

#define getPropertyBoolean(prompt)     \
    do { \
        value = OSDynamicCast(OSBoolean, dict->getObject(prompt));   \
        if (value == nullptr) { \
            AlwaysLog(valueInvalid, prompt);  \
            continue;   \
        } \
    } while (0)

#define getPropertyNumber(prompt)     \
    do { \
        value = OSDynamicCast(OSNumber, dict->getObject(prompt));   \
        if (value == nullptr) { \
            AlwaysLog(valueInvalid, prompt);  \
            continue;   \
        } \
    } while (0)

#define getPropertyString(prompt)     \
    do { \
        value = OSDynamicCast(OSString, dict->getObject(prompt));   \
        if (value == nullptr) { \
            AlwaysLog(valueInvalid, prompt);  \
            continue;   \
        } \
    } while (0)

#define setPropertyBoolean(dict, name, boolean) \
    do { dict->setObject(name, boolean ? kOSBooleanTrue : kOSBooleanFalse); } while (0)

// define a OSNumber(OSObject) *value before use
#define setPropertyNumber(dict, name, number, bits) \
    do { \
        value = OSNumber::withNumber(number, bits); \
        if (value != nullptr) { \
            dict->setObject(name, value); \
            value->release(); \
        } \
    } while (0)

#define setPropertyString(dict, name, str) \
    do { \
        value = OSString::withCString(str); \
        if (value != nullptr) { \
            dict->setObject(name, value); \
            value->release(); \
        } \
    } while (0)

#define setPropertyBytes(dict, name, data, len) \
    do { \
        value = OSData::withBytes(data, len); \
        if (value != nullptr) { \
            dict->setObject(name, value); \
            value->release(); \
        } \
    } while (0)

#endif /* common_h */
