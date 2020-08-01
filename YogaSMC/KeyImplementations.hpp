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

class VPCKey : public VirtualSMCValue {
protected:
    IOService *vpc;
public:
    VPCKey(IOService *src=0) : vpc(src) {};
};

class BDVT : public VPCKey { using VPCKey::VPCKey; protected: SMC_RESULT readAccess() override; SMC_RESULT writeAccess() override; bool value {false}; int readcount {0}; int writecount {0};};

class CH0B : public VirtualSMCValue { protected: SMC_RESULT readAccess() override; SMC_RESULT writeAccess() override; SMC_DATA value {0};};

#endif /* KeyImplementations_hpp */
