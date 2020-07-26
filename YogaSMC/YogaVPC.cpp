//  SPDX-License-Identifier: GPL-2.0-only
//
//  YogaVPC.cpp
//  YogaSMC
//
//  Created by Zhen on 2020/7/10.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "YogaVPC.hpp"

OSDefineMetaClassAndStructors(YogaVPC, IOService);

IOService *YogaVPC::probe(IOService *provider, SInt32 *score)
{
    IOLog("%s::%s Probing\n", getName(), provider->getName());

    vpc = OSDynamicCast(IOACPIPlatformDevice, provider);
    if (!vpc)
        return nullptr;

    return super::probe(provider, score);
}

bool YogaVPC::start(IOService *provider) {
    bool res = super::start(provider);

    name = provider->getName();

    IOLog("%s::%s Starting\n", getName(), name);

    if (!initVPC())
        return false;

    workLoop = IOWorkLoop::workLoop();
    commandGate = IOCommandGate::commandGate(this);
    if (!workLoop || !commandGate || (workLoop->addEventSource(commandGate) != kIOReturnSuccess)) {
        IOLog("%s::%s Failed to add commandGate\n", getName(), name);
        return false;
    }

    updateAll();

    setProperty("RM,deliverNotifications", kOSBooleanTrue);
    registerService();
    return res;
}

void YogaVPC::stop(IOService *provider) {
    IOLog("%s::%s Stopping\n", getName(), name);
    if (clamshellMode) {
        IOLog("%s::%s Disabling clamshell mode\n", getName(), name);
        toggleClamshell();
    }

    workLoop->removeEventSource(commandGate);
    OSSafeReleaseNULL(commandGate);
    OSSafeReleaseNULL(workLoop);

    terminate();
    detach(provider);
    super::stop(provider);
}

YogaVPC* YogaVPC::withDevice(IOACPIPlatformDevice *device, OSString *pnp) {
    YogaVPC* vpc = OSTypeAlloc(YogaVPC);

    OSDictionary* dictionary = OSDictionary::withCapacity(1);
    dictionary->setObject("Type", pnp);
    
    vpc->vpc = device;

    if (!vpc->init(dictionary) ||
        !vpc->attach(device) ||
        !vpc->start(device)) {
        OSSafeReleaseNULL(vpc);
    }

    dictionary->release();

    return vpc;
}

void YogaVPC::updateAll() {
    updateClamshell();
    updateVPC();
}

void YogaVPC::setPropertiesGated(OSObject* props) {
    if (!vpc) {
        IOLog(VPCUnavailable, getName(), name);
        return;
    }

    OSDictionary* dict = OSDynamicCast(OSDictionary, props);
    if (!dict)
        return;

//    IOLog("%s: %d objects in properties\n", getName(), name, dict->getCount());
    OSCollectionIterator* i = OSCollectionIterator::withCollection(dict);

    if (i != NULL) {
        while (OSString* key = OSDynamicCast(OSString, i->getNextObject())) {
            if (key->isEqualTo(clamshellPrompt)) {
                OSBoolean * value = OSDynamicCast(OSBoolean, dict->getObject(clamshellPrompt));
                if (value == NULL) {
                    IOLog(valueInvalid, getName(), name, clamshellPrompt);
                    continue;
                }

                updateClamshell(false);

                if (value->getValue() == clamshellMode) {
                    IOLog(valueMatched, getName(), name, clamshellPrompt, (clamshellMode ? "enabled" : "disabled"));
                } else {
                    toggleClamshell();
                }
            } else if (key->isEqualTo(updatePrompt)) {
                updateAll();
            } else {
                IOLog("%s::%s Unknown property %s\n", getName(), name, key->getCStringNoCopy());
            }
        }
        i->release();
    }

    return;
}

IOReturn YogaVPC::setProperties(OSObject *props) {
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &YogaVPC::setPropertiesGated), props);
    return kIOReturnSuccess;
}

IOReturn YogaVPC::message(UInt32 type, IOService *provider, void *argument) {
    if (argument) {
        IOLog("%s::%s message: type=%x, provider=%s, argument=0x%04x\n", getName(), name, type, provider->getName(), *((UInt32 *) argument));
        updateAll();
    } else {
        IOLog("%s::%s message: type=%x, provider=%s\n", getName(), name, type, provider->getName());
    }
    return kIOReturnSuccess;
}

bool YogaVPC::updateClamshell(bool update) {
    UInt32 state;

    if (vpc->evaluateInteger(getClamshellMode, &state) != kIOReturnSuccess) {
        IOLog(updateFailure, getName(), name, clamshellPrompt);
        return false;
    }

    clamshellMode = state ? true : false;

    if (update) {
        IOLog(updateSuccess, getName(), name, clamshellPrompt, state);
        setProperty(clamshellPrompt, clamshellMode);
    }

    return true;
}

bool YogaVPC::toggleClamshell() {
    UInt32 result;

    OSObject* params[1] = {
        OSNumber::withNumber(!clamshellMode, 32)
    };

    if (vpc->evaluateInteger(setClamshellMode, &result, params, 1) != kIOReturnSuccess || result != 0) {
        IOLog(toggleFailure, getName(), name, clamshellPrompt);
        return false;
    }

    clamshellMode = !clamshellMode;
    IOLog(toggleSuccess, getName(), name, clamshellPrompt, clamshellMode, (clamshellMode ? "on" : "off"));
    setProperty(clamshellPrompt, clamshellMode);

    return true;
}
