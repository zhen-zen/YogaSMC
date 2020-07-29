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
#include "IdeaVPC.hpp"

static constexpr SMC_KEY KeyBDVT = SMC_MAKE_IDENTIFIER('B','D','V','T');

class BDVT : public VirtualSMCValue {
    protected: SMC_RESULT readAccess() override; SMC_RESULT writeAccess() override; bool value {false}; IdeaVPC *vpc;
    public: inline BDVT(IdeaVPC *src=0) {vpc = src;};
};

#endif /* KeyImplementations_hpp */
