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

void IdeaWMI::stop(IOService *provider) {
    if (dev)
        dev->stop(vpc);
    super::stop(provider);
}

void IdeaWMI::free(void)
{
    if (dev)
        dev->free();
    super::free();
}

void IdeaWMI::YogaEvent(UInt32 argument) {
    switch (argument) {
        case kIOACPIMessageReserved:
            IOLog("%s::%s Type 80 -> WMI2\n", getName(), name);
            // force enable keyboard and touchpad
            setTopCase(true);
            if (dev) {
                dev->updateKeyboard();
                dev->toggleFnlock();
            }
            break;

        default:
            IOLog("%s::%s Unknown argument 0x%x\n", getName(), name, argument);
            break;
    }
}

bool IdeaWMI::initVPC() {
    dev = IdeaVPC::withDevice(vpc, getPnp());
    return true;
}

IOReturn IdeaWMI::setPowerState(unsigned long powerState, IOService *whatDevice){
    IOLog("%s::%s powerState %ld : %s", getName(), name, powerState, powerState ? "on" : "off");

    if (whatDevice != this)
        return kIOReturnInvalid;

    // Maybe move to IdeaVPC itself
    if (dev) {
        if (dev->BacklightCap && dev->AutomaticBacklightMode) {
            dev->updateKeyboard();
            if (powerState == 0) {
                dev->BacklightModeSleep = dev->BacklightMode;
                if (dev->BacklightMode)
                    dev->toggleBacklight();
            } else {
                if (dev->BacklightModeSleep && !dev->BacklightMode)
                    dev->toggleBacklight();
            }
        }
    }
    return super::setPowerState(powerState, whatDevice);
}
