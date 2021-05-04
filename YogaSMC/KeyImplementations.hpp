//
//  KeyImplementations.hpp
//  YogaSMC
//
//  Created by Zhen on 7/28/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef KeyImplementations_hpp
#define KeyImplementations_hpp

#include <libkern/libkern.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <VirtualSMCSDK/kern_vsmcapi.hpp>
#include "YogaBaseService.hpp"

#define addECKeySp(key, name, type) \
    do { \
        if (sensorCount < MAX_SENSOR && (method = OSDynamicCast(OSString, conf->getObject(name))) && (method->getLength() == 4)) { \
            if (ec->validateObject(method->getCStringNoCopy()) == kIOReturnSuccess) { \
                atomic_init(&currentSensor[sensorCount], 0); \
                VirtualSMCAPI::addKey(key, vsmcPlugin.data, VirtualSMCAPI::valueWithSp(0, SmcKeyTypeSp78, new type(&currentSensor[sensorCount]))); \
                sensorMethods[sensorCount++] = method->getCStringNoCopy(); \
                status->setObject(name, kOSBooleanTrue); \
            } else { \
                status->setObject(name, kOSBooleanFalse); \
            } \
        } \
    } while (0)

struct sensorPair {
    const SMC_KEY key;
    const char *name;
};

static constexpr const char *KeyIndexes = "0123456789ABCDEF";

static constexpr SMC_KEY KeyBDVT = SMC_MAKE_IDENTIFIER('B','D','V','T');
static constexpr SMC_KEY KeyCH0B = SMC_MAKE_IDENTIFIER('C','H','0','B');
static constexpr SMC_KEY KeyTA0P(size_t i) { return SMC_MAKE_IDENTIFIER('T','A',KeyIndexes[i],'P');};
static constexpr SMC_KEY KeyTB0T(size_t i) { return SMC_MAKE_IDENTIFIER('T','B',KeyIndexes[i],'T'); }
static constexpr SMC_KEY KeyTCGC = SMC_MAKE_IDENTIFIER('T','C','G','C');
static constexpr SMC_KEY KeyTCHP = SMC_MAKE_IDENTIFIER('T','C','H','P');
static constexpr SMC_KEY KeyTCSA = SMC_MAKE_IDENTIFIER('T','C','S','A');
static constexpr SMC_KEY KeyTCXC = SMC_MAKE_IDENTIFIER('T','C','X','C');
static constexpr SMC_KEY KeyTG0P(size_t i) { return SMC_MAKE_IDENTIFIER('T','G',KeyIndexes[i],'P');};
static constexpr SMC_KEY KeyTH0P(size_t i) { return SMC_MAKE_IDENTIFIER('T','H',KeyIndexes[i],'P');};
static constexpr SMC_KEY KeyTH0a = SMC_MAKE_IDENTIFIER('T','H','0','a');
static constexpr SMC_KEY KeyTH0b = SMC_MAKE_IDENTIFIER('T','H','0','b');
static constexpr SMC_KEY KeyTM0P = SMC_MAKE_IDENTIFIER('T','M','0','P');
static constexpr SMC_KEY KeyTM0p(size_t i) { return SMC_MAKE_IDENTIFIER('T','M',KeyIndexes[i],'p'); }
static constexpr SMC_KEY KeyTPCD = SMC_MAKE_IDENTIFIER('T','P','C','D');
static constexpr SMC_KEY KeyTTRD = SMC_MAKE_IDENTIFIER('T','T','R','D');
static constexpr SMC_KEY KeyTW0P = SMC_MAKE_IDENTIFIER('T','W','0','P');
static constexpr SMC_KEY KeyTaLC = SMC_MAKE_IDENTIFIER('T','a','L','C');
static constexpr SMC_KEY KeyTaRC = SMC_MAKE_IDENTIFIER('T','a','R','C');
static constexpr SMC_KEY KeyTh0H(size_t i) { return SMC_MAKE_IDENTIFIER('T','h',KeyIndexes[i],'H'); }
static constexpr SMC_KEY KeyTs0P(size_t i) { return SMC_MAKE_IDENTIFIER('T','s',KeyIndexes[i],'P'); }

// Fan related keys, from SMCDellSensors

static constexpr SMC_KEY KeyFNum = SMC_MAKE_IDENTIFIER('F','N','u','m'); // Number of supported fans
static constexpr SMC_KEY KeyF0Ac(size_t i) { return SMC_MAKE_IDENTIFIER('F',KeyIndexes[i],'A','c'); } // Actual RPM
static constexpr SMC_KEY KeyF0ID(size_t i) { return SMC_MAKE_IDENTIFIER('F',KeyIndexes[i],'I','D'); } // Description
static constexpr SMC_KEY KeyF0Md(size_t i) { return SMC_MAKE_IDENTIFIER('F',KeyIndexes[i],'M','d'); } // Manual Mode (New)
static constexpr SMC_KEY KeyF0Mn(size_t i) { return SMC_MAKE_IDENTIFIER('F',KeyIndexes[i],'M','n'); } // Minimum RPM
static constexpr SMC_KEY KeyF0Mx(size_t i) { return SMC_MAKE_IDENTIFIER('F',KeyIndexes[i],'M','x'); } // Maximum RPM
static constexpr SMC_KEY KeyF0Sf(size_t i) { return SMC_MAKE_IDENTIFIER('F',KeyIndexes[i],'S','f'); } // Safe RPM
static constexpr SMC_KEY KeyF0Tg(size_t i) { return SMC_MAKE_IDENTIFIER('F',KeyIndexes[i],'T','g'); } // Target speed
static constexpr SMC_KEY KeyFS__ = SMC_MAKE_IDENTIFIER('F','S','!',' '); // Fan force bits. FS![15:0] Setting bit to 1 allows for external control over fan speed target and prevents thermal manager from actively overidding value set via key access.

typedef enum { FAN_PWM_TACH, FAN_RPM, PUMP_PWM, PUMP_RPM, FAN_PWM_NOTACH, EMPTY_PLACEHOLDER } FanType;

typedef enum {
    LEFT_LOWER_FRONT, CENTER_LOWER_FRONT, RIGHT_LOWER_FRONT,
    LEFT_MID_FRONT,   CENTER_MID_FRONT,   RIGHT_MID_FRONT,
    LEFT_UPPER_FRONT, CENTER_UPPER_FRONT, RIGHT_UPPER_FRONT,
    LEFT_LOWER_REAR,  CENTER_LOWER_REAR,  RIGHT_LOWER_REAR,
    LEFT_MID_REAR,    CENTER_MID_REAR,    RIGHT_MID_REAR,
    LEFT_UPPER_REAR,  CENTER_UPPER_REAR,  RIGHT_UPPER_REAR
} LocationType;

static constexpr int32_t DiagFunctionStrLen = 12;

typedef struct fanTypeDescStruct {
    UInt8 type         {FAN_PWM_TACH};
    UInt8 ui8Zone     {1};
    UInt8 location     {LEFT_MID_REAR};
    UInt8 rsvd        {0}; // padding to get us to 16 bytes
    char  strFunction[DiagFunctionStrLen];
} FanTypeDescStruct;

class atomicSpKey : public VirtualSMCValue {
    _Atomic(uint32_t) *currentSensor;
protected:
    SMC_RESULT readAccess() override;
public:
    atomicSpKey(_Atomic(uint32_t) *currentSensor) : currentSensor(currentSensor) {};
};

class atomicSpDeciKelvinKey : public VirtualSMCValue {
    _Atomic(uint32_t) *currentSensor;
protected:
    SMC_RESULT readAccess() override;
public:
    atomicSpDeciKelvinKey(_Atomic(uint32_t) *currentSensor) : currentSensor(currentSensor) {};
};

class atomicFltKey : public VirtualSMCValue {
    _Atomic(uint32_t) *currentSensor;
protected:
    SMC_RESULT readAccess() override;
public:
    atomicFltKey(_Atomic(uint32_t) *currentSensor) : currentSensor(currentSensor) {};
};

class atomicFpKey : public VirtualSMCValue {
    _Atomic(uint32_t) *currentSensor;
protected:
    SMC_RESULT readAccess() override;
public:
    atomicFpKey(_Atomic(uint32_t) *currentSensor) : currentSensor(currentSensor) {};
};

class messageKey : public VirtualSMCValue {
protected:
    YogaBaseService *drv;
public:
    messageKey(YogaBaseService *src=nullptr) : drv(src) {};
};

class BDVT : public messageKey { using messageKey::messageKey; protected: SMC_RESULT writeAccess() override; SMC_RESULT update(const SMC_DATA *src) override;};
class CH0B : public VirtualSMCValue { protected: SMC_RESULT writeAccess() override; SMC_RESULT update(const SMC_DATA *src) override;};

#endif /* KeyImplementations_hpp */
