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

#define addECKeySp(key, method) \
    do { \
        if (ec->validateObject(method) == kIOReturnSuccess) { \
            setProperty(method, true); \
            VirtualSMCAPI::addKey(key, vsmcPlugin.data, VirtualSMCAPI::valueWithSp(0, SmcKeyTypeSp78, new simpleECKey(ec, method))); \
        } else { \
            setProperty(method, false); \
        } \
    } while (0)

static constexpr SMC_KEY KeyBDVT = SMC_MAKE_IDENTIFIER('B','D','V','T');
static constexpr SMC_KEY KeyCH0B = SMC_MAKE_IDENTIFIER('C','H','0','B');

static constexpr SMC_KEY KeyTPCD = SMC_MAKE_IDENTIFIER('T','P','C','D');
static constexpr SMC_KEY KeyTaLC = SMC_MAKE_IDENTIFIER('T','a','L','C');
static constexpr SMC_KEY KeyTaRC = SMC_MAKE_IDENTIFIER('T','a','R','C');

class simpleECKey : public VirtualSMCValue {
protected:
    IOACPIPlatformDevice *ec;
    const char* method;
    SMC_RESULT readAccess() override;
public:
    simpleECKey(IOACPIPlatformDevice *ec, const char* method) : ec(ec), method(method) {};
};

class messageKey : public VirtualSMCValue {
protected:
    IOService *dst;
public:
    messageKey(IOService *src=nullptr) : dst(src) {};
};

class BDVT : public messageKey { using messageKey::messageKey; protected: SMC_RESULT writeAccess() override; SMC_RESULT update(const SMC_DATA *src) override;};
class CH0B : public VirtualSMCValue { protected: SMC_RESULT writeAccess() override; SMC_RESULT update(const SMC_DATA *src) override;};

#endif /* KeyImplementations_hpp */
