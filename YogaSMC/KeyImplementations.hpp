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
#include "YogaSMC.hpp"

#define addECKeySp(key, name) \
    do { \
        if (sensorCount < MAX_SENSOR && (method = OSDynamicCast(OSString, conf->getObject(name))) && (method->getLength() == 4)) { \
            if (ec->validateObject(method->getCStringNoCopy()) == kIOReturnSuccess) { \
                atomic_init(&currentSensor[sensorCount], 0); \
                VirtualSMCAPI::addKey(key, vsmcPlugin.data, VirtualSMCAPI::valueWithSp(0, SmcKeyTypeSp78, new atomicECKey(&currentSensor[sensorCount]))); \
                sensorMethod[sensorCount] = method->getCStringNoCopy(); \
                status->setObject(name, kOSBooleanTrue); \
                sensorCount++; \
            } else { \
                status->setObject(name, kOSBooleanFalse); \
            } \
        } \
    } while (0)

static constexpr const char *KeyIndexes = "0123456789ABCDEF";

static constexpr SMC_KEY KeyBDVT = SMC_MAKE_IDENTIFIER('B','D','V','T');
static constexpr SMC_KEY KeyCH0B = SMC_MAKE_IDENTIFIER('C','H','0','B');
static constexpr SMC_KEY KeyTCSA = SMC_MAKE_IDENTIFIER('T','C','S','A');
static constexpr SMC_KEY KeyTCXC = SMC_MAKE_IDENTIFIER('T','C','X','C');
static constexpr SMC_KEY KeyTM0P = SMC_MAKE_IDENTIFIER('T','M','0','P');
static constexpr SMC_KEY KeyTM0p(size_t i) { return SMC_MAKE_IDENTIFIER('T','M',KeyIndexes[i],'p'); }
static constexpr SMC_KEY KeyTPCD = SMC_MAKE_IDENTIFIER('T','P','C','D');
static constexpr SMC_KEY KeyTW0P = SMC_MAKE_IDENTIFIER('T','W','0','P');
static constexpr SMC_KEY KeyTaLC = SMC_MAKE_IDENTIFIER('T','a','L','C');
static constexpr SMC_KEY KeyTaRC = SMC_MAKE_IDENTIFIER('T','a','R','C');
static constexpr SMC_KEY KeyTh0H(size_t i) { return SMC_MAKE_IDENTIFIER('T','h',KeyIndexes[i],'H'); }
static constexpr SMC_KEY KeyTs0p(size_t i) { return SMC_MAKE_IDENTIFIER('T','s',KeyIndexes[i],'p'); }

class YogaSMC;

class atomicECKey : public VirtualSMCValue {
    _Atomic(uint32_t) *currentSensor;
protected:
    SMC_RESULT readAccess() override;
public:
    atomicECKey(_Atomic(uint32_t) *currentSensor) : currentSensor(currentSensor) {};
};

class messageKey : public VirtualSMCValue {
protected:
    YogaSMC *drv;
public:
    messageKey(YogaSMC *src=nullptr) : drv(src) {};
};

class BDVT : public messageKey { using messageKey::messageKey; protected: SMC_RESULT writeAccess() override; SMC_RESULT update(const SMC_DATA *src) override;};
class CH0B : public VirtualSMCValue { protected: SMC_RESULT writeAccess() override; SMC_RESULT update(const SMC_DATA *src) override;};

#endif /* KeyImplementations_hpp */
