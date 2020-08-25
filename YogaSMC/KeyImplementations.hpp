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
#include <VirtualSMCSDK/kern_vsmcapi.hpp>

static constexpr SMC_KEY KeyBDVT = SMC_MAKE_IDENTIFIER('B','D','V','T');
static constexpr SMC_KEY KeyCH0B = SMC_MAKE_IDENTIFIER('C','H','0','B');

class ECKey : public VirtualSMCValue {
protected:
    IOService *dst;
public:
    ECKey(IOService *src=nullptr) : dst(src) {};
};

class BDVT : public ECKey { using ECKey::ECKey; protected: SMC_RESULT writeAccess() override; SMC_RESULT update(const SMC_DATA *src) override; int counteru {0}; int counterw {0};};
class CH0B : public VirtualSMCValue { protected: SMC_RESULT writeAccess() override; SMC_RESULT update(const SMC_DATA *src) override; int counteru {0}; int counterw {0};};

#endif /* KeyImplementations_hpp */
