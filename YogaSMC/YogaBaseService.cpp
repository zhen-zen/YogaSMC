//
//  YogaBaseService.cpp
//  YogaSMC
//
//  Created by Zhen on 10/17/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "YogaBaseService.hpp"

OSDefineMetaClassAndStructors(YogaBaseService, IOService)

bool YogaBaseService::init(OSDictionary *dictionary)
{
    if (!super::init(dictionary))
        return false;

    DebugLog("Initializing");

    _deliverNotification = OSSymbol::withCString(kDeliverNotifications);
    if (!_deliverNotification)
        return false;

    _notificationServices = OSSet::withCapacity(1);

    return true;
}

IOService *YogaBaseService::probe(IOService *provider, SInt32 *score)
{
    iname = provider->getName();
    return this;
}

bool YogaBaseService::start(IOService *provider)
{
#ifndef ALTER
    setProperty("VersionInfo", kextVersion);
#else
    extern kmod_info_t kmod_info;
    setProperty("VersionInfo", kmod_info.version);
    setProperty("Variant", "Alter");
#endif
    workLoop = IOWorkLoop::workLoop();
    commandGate = IOCommandGate::commandGate(this);
    if (!workLoop || !commandGate || (workLoop->addEventSource(commandGate) != kIOReturnSuccess)) {
        AlwaysLog("Failed to add commandGate");
        return false;
    }

    OSDictionary * propertyMatch = propertyMatching(_deliverNotification, kOSBooleanTrue);
    if (propertyMatch) {
      IOServiceMatchingNotificationHandler notificationHandler = OSMemberFunctionCast(IOServiceMatchingNotificationHandler, this, &YogaBaseService::notificationHandler);

      //
      // Register notifications for availability of any IOService objects wanting to consume our message events
      //
      _publishNotify = addMatchingNotification(gIOFirstPublishNotification,
                                             propertyMatch,
                                             notificationHandler,
                                             this,
                                             0, 10000);

      _terminateNotify = addMatchingNotification(gIOTerminatedNotification,
                                               propertyMatch,
                                               notificationHandler,
                                               this,
                                               0, 10000);

      propertyMatch->release();
    }

    if (isPMsupported) {
        PMinit();
        provider->joinPMtree(this);
        registerPowerDriver(this, IOPMPowerStates, kIOPMNumberPowerStates);
    }

    return true;
}

void YogaBaseService::stop(IOService *provider)
{
    _publishNotify->remove();
    _terminateNotify->remove();
    _notificationServices->flushCollection();
    OSSafeReleaseNULL(_notificationServices);
    OSSafeReleaseNULL(_deliverNotification);

    workLoop->removeEventSource(commandGate);
    OSSafeReleaseNULL(commandGate);
    OSSafeReleaseNULL(workLoop);

    if (isPMsupported)
        PMstop();

    super::stop(provider);
}

void YogaBaseService::dispatchMessageGated(int* message, void* data)
{
    OSCollectionIterator* i = OSCollectionIterator::withCollection(_notificationServices);

    if (i) {
        while (IOService* service = OSDynamicCast(IOService, i->getNextObject())) {
            service->message(*message, this, data);
        }
        i->release();
    }
}

void YogaBaseService::dispatchMessage(int message, void* data)
{
    if (_notificationServices->getCount() == 0) {
        AlwaysLog("No available notification consumer");
        return;
    }
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &YogaBaseService::dispatchMessageGated), &message, data);
}

void YogaBaseService::notificationHandlerGated(IOService *newService, IONotifier *notifier)
{
    if (notifier == _publishNotify) {
        DebugLog("Notification consumer published: %s", newService->getName());
        _notificationServices->setObject(newService);
    }

    if (notifier == _terminateNotify) {
        DebugLog("Notification consumer terminated: %s", newService->getName());
        _notificationServices->removeObject(newService);
    }
}

bool YogaBaseService::notificationHandler(void *refCon, IOService *newService, IONotifier *notifier)
{
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &YogaBaseService::notificationHandlerGated), newService, notifier);
    return true;
}

bool YogaBaseService::findPNP(const char *id, IOACPIPlatformDevice **dev) {
    DebugLog("findPNP for %s", id);
    auto pnp = OSString::withCString(id);
    if ((*dev) && (*dev)->compareName(pnp, nullptr)) {
        DebugLog("found %s at %s", id, (*dev)->getName());
    } else {
        *dev = nullptr;
        auto iterator = IORegistryIterator::iterateOver(gIOACPIPlane, kIORegistryIterateRecursively);
        if (!iterator) {
            AlwaysLog("findPNP failed to create iterator");
        } else {
            while (auto entry = iterator->getNextObject()) {
                if (entry->compareName(pnp, nullptr)) {
                    DebugLog("found %s at %s", id, entry->getName());
                    if ((*dev = OSDynamicCast(IOACPIPlatformDevice, entry)))
                        break;
                }
            }
            iterator->release();
        }
    }
    pnp->release();
    return !!(*dev);
}

void YogaBaseService::toggleTouchpad() {
    dispatchMessage(kSMC_getDisableTouchpad, &TouchPadenabled);
    TouchPadenabled = !TouchPadenabled;
    dispatchMessage(kSMC_setDisableTouchpad, &TouchPadenabled);
    DebugLog("TouchPad Input %s", TouchPadenabled ? "enabled" : "disabled");
    setProperty("TouchPadEnabled", TouchPadenabled);
}

void YogaBaseService::toggleKeyboard() {
    dispatchMessage(kSMC_getKeyboardStatus, &Keyboardenabled);
    Keyboardenabled = !Keyboardenabled;
    dispatchMessage(kSMC_setKeyboardStatus, &Keyboardenabled);
    DebugLog("Keyboard Input %s", Keyboardenabled ? "enabled" : "disabled");
    setProperty("KeyboardEnabled", Keyboardenabled);
}

void YogaBaseService::setTopCase(bool enable) {
    dispatchMessage(kSMC_setKeyboardStatus, &enable);
    dispatchMessage(kSMC_setDisableTouchpad, &enable);
    DebugLog("TopCase Input %s", enable ? "enabled" : "disabled");
    setProperty("TopCaseEnabled", enable);
}

bool YogaBaseService::updateTopCase() {
    dispatchMessage(kSMC_getKeyboardStatus, &Keyboardenabled);
    dispatchMessage(kSMC_getDisableTouchpad, &TouchPadenabled);
    if (Keyboardenabled != TouchPadenabled) {
        AlwaysLog("status mismatch: %d, %d", Keyboardenabled, TouchPadenabled);
        return false;
    }
    return true;
}

IOReturn YogaBaseService::setPowerState(unsigned long powerStateOrdinal, IOService * whatDevice) {
    DebugLog("powerState %ld : %s", powerStateOrdinal, powerStateOrdinal ? "on" : "off");
    if (whatDevice != this)
        return kIOReturnInvalid;

    return kIOPMAckImplied;
}

IOReturn YogaBaseService::readECName(const char* name, UInt32 *result) {
    if (!ec) return kIOReturnNoDevice;
    IOReturn ret = ec->evaluateInteger(name, result);
    switch (ret) {
        case kIOReturnSuccess:
            break;

        case kIOReturnBadArgument:
            AlwaysLog("read %s failed, bad argument (field size too large?)", name);
            break;
            
        default:
            AlwaysLog("read %s failed %x", name, ret);
            break;
    }
    return ret;
}

IOReturn YogaBaseService::method_re1b(UInt32 offset, UInt8 *result) {
    if (!ec || !(ECAccessCap & ECReadCap))
        return kIOReturnUnsupported;

    UInt32 raw;
    OSObject* params[1] = {
        OSNumber::withNumber(offset, 32)
    };

    IOReturn ret = ec->evaluateInteger(readECOneByte, &raw, params, 1);
    params[0]->release();

    if (ret != kIOReturnSuccess)
        AlwaysLog("read 0x%02x failed", offset);

    *result = raw;
    return ret;
}

IOReturn YogaBaseService::method_recb(UInt32 offset, UInt32 size, OSData **data) {
    if (!ec || !(ECAccessCap & BIT(0)))
        return kIOReturnUnsupported;

    // Arg0 - offset in bytes from zero-based EC
    // Arg1 - size of buffer in bits
    OSObject* params[2] = {
        OSNumber::withNumber(offset, 32),
        OSNumber::withNumber(size * 8, 32)
    };
    OSObject* result;

    IOReturn ret = ec->evaluateObject(readECBytes, &result, params, 2);
    params[0]->release();
    params[1]->release();

    if (ret != kIOReturnSuccess || !(*data = OSDynamicCast(OSData, result))) {
        AlwaysLog("read %d bytes @ 0x%02x failed", size, offset);
        OSSafeReleaseNULL(result);
        return kIOReturnInvalid;
    }

    if ((*data)->getLength() != size) {
        AlwaysLog("read %d bytes @ 0x%02x, got %d bytes", size, offset, (*data)->getLength());
        OSSafeReleaseNULL(result);
        return kIOReturnNoBandwidth;
    }

    return ret;
}

IOReturn YogaBaseService::method_we1b(UInt32 offset, UInt8 value) {
    if (!ec || !(ECAccessCap & ECWriteCap))
        return kIOReturnUnsupported;

    OSObject* params[2] = {
        OSNumber::withNumber(offset, 32),
        OSNumber::withNumber(value, 8)
    };

    IOReturn ret = ec->evaluateObject(writeECOneByte, nullptr, params, 2);
    params[0]->release();
    params[1]->release();

    if (ret != kIOReturnSuccess)
        AlwaysLog("write 0x%02x @ 0x%02x failed", value, offset);
    else
        DebugLog("write 0x%02x @ 0x%02x success", value, offset);
    return ret;
}

void YogaBaseService::validateEC() {
    if (!ec)
        return;
    if (ec->validateObject(readECOneByte) != kIOReturnSuccess ||
        ec->validateObject(readECBytes) != kIOReturnSuccess) {
        setProperty("EC Capability", "Basic");
        return;
    }
    ECAccessCap |= ECReadCap;
    if (ec->validateObject(writeECOneByte) != kIOReturnSuccess) {
        setProperty("EC Capability", "RO");
        return;
    }
    ECAccessCap |= ECWriteCap;
    setProperty("EC Capability", "RW");
}

void YogaBaseService::dispatchKeyEvent(UInt16 keyCode, bool goingDown, bool time) {
    PS2KeyInfo info = {.adbKeyCode = keyCode, .goingDown = goingDown, .eatKey = false};
    clock_get_uptime(&info.time);
    if (time)
        dispatchMessage(kPS2M_notifyKeyTime, &info.time);
    dispatchMessage(kSMC_notifyKeystroke, &info);
}
