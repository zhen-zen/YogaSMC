//  SPDX-License-Identifier: GPL-2.0-only
//
//  IdeaWMI.hpp
//  YogaSMC
//
//  Created by Zhen on 2020/6/23.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef IdeaWMI_hpp
#define IdeaWMI_hpp

#include "../YogaSMC.hpp"
#include "../YogaVPC/IdeaVPC.hpp"

class IdeaWMI : public YogaWMI
{
    typedef YogaWMI super;
    OSDeclareDefaultStructors(IdeaWMI)

private:
    bool initVPC() override;
    inline OSString *getPnp() override {return OSString::withCString(PnpDeviceIdVPCIdea);};

    void YogaEvent(UInt32 argument) override;

    IdeaVPC *dev;

public:
    IOService *probe(IOService *provider, SInt32 *score) override;
    void stop(IOService *provider) override;
    void free(void) override;
};

#endif /* IdeaWMI_hpp */
