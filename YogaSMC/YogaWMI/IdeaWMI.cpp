//  SPDX-License-Identifier: GPL-2.0-only
//
//  IdeaWMI.cpp
//  YogaSMC
//
//  Created by Zhen on 2020/6/23.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "IdeaWMI.hpp"
OSDefineMetaClassAndStructors(IdeaWMI, YogaWMI);

IOService *IdeaWMI::probe(IOService *provider, SInt32 *score)
{
    if (/* DISABLES CODE */ (false)) {
        IOLog("not a valid IdeaPad WMI interface");
        return NULL;
    }
    // TODO: identify an appropritate interface to attach

    return super::probe(provider, score);
}

void IdeaWMI::ACPIEvent(UInt32 argument) {
    switch (argument) {
        case kIOACPIMessageReserved:
            IOLog("%s::%s Notification 80\n", getName(), name);
            // force enable keyboard and touchpad
            setTopCase(true);
            dispatchMessage(kSMC_FnlockEvent, NULL);
            break;

        default:
            IOLog("%s::%s Unknown ACPI Notification 0x%x\n", getName(), name, argument);
            break;
    }
}
