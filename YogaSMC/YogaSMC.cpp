//
//  YogaSMC.cpp
//  YogaSMC
//
//  Created by Zhen on 7/29/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#include "YogaSMC.hpp"

OSDefineMetaClassAndStructors(YogaSMC, IOService);

bool ADDPR(debugEnabled) = true;
uint32_t ADDPR(debugPrintDelay) = 0;

bool YogaSMC::init(OSDictionary *dictionary)
{
    bool res = super::init(dictionary);
    IOLog("%s Initializing\n", getName());

    _deliverNotification = OSSymbol::withCString(kDeliverNotifications);
     if (!_deliverNotification)
        return false;

    _notificationServices = OSSet::withCapacity(1);

    extern kmod_info_t kmod_info;
    setProperty("YogaSMC,Build", __DATE__);
    setProperty("YogaSMC,Version", kmod_info.version);

    return res;
}

IOService *YogaSMC::probe(IOService *provider, SInt32 *score)
{
    if (provider->getClient() != this) {
        IOLog("%s already loaded, exiting\n", provider->getClient()->getName());
        return nullptr;
    }

    IOLog("%s Probing\n", getName());

    IOACPIPlatformDevice *vpc {nullptr};

    if(getVPCName() && !findPNP(getVPCName(), &vpc)) {
        IOLog("%s Failed to find compatible VPC %s\n", getName(), getVPCName());
        return nullptr;
    }

    if (!findPNP(PnpDeviceIdEC, &ec))
        name = "";
    else
        name = ec->getName();

    return super::probe(provider, score);
}

bool YogaSMC::start(IOService *provider) {
    bool res = super::start(provider);

    IOLog("%s::%s Starting\n", getName(), name);

    workLoop = IOWorkLoop::workLoop();
    commandGate = IOCommandGate::commandGate(this);
    if (!workLoop || !commandGate || (workLoop->addEventSource(commandGate) != kIOReturnSuccess)) {
        IOLog("%s::%s Failed to add commandGate\n", getName(), name);
        return false;
    }

    OSDictionary * propertyMatch = propertyMatching(_deliverNotification, kOSBooleanTrue);
    if (propertyMatch) {
        IOServiceMatchingNotificationHandler notificationHandler = OSMemberFunctionCast(IOServiceMatchingNotificationHandler, this, &YogaSMC::notificationHandler);

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

    VirtualSMCAPI::addKey(KeyBDVT, vsmcPlugin.data, VirtualSMCAPI::valueWithFlag(false, new BDVT(this), SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE));
    VirtualSMCAPI::addKey(KeyCH0B, vsmcPlugin.data, VirtualSMCAPI::valueWithFlag(false, new CH0B, SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE));
    vsmcNotifier = VirtualSMCAPI::registerHandler(vsmcNotificationHandler, this);

    registerService();
    return res;
}

void YogaSMC::stop(IOService *provider)
{
    IOLog("%s::%s Stopping\n", getName(), name);

    _publishNotify->remove();
    _terminateNotify->remove();
    _notificationServices->flushCollection();
    OSSafeReleaseNULL(_notificationServices);
    OSSafeReleaseNULL(_deliverNotification);

    workLoop->removeEventSource(commandGate);
    OSSafeReleaseNULL(commandGate);
    OSSafeReleaseNULL(workLoop);

    terminate();
    PMstop();

    super::stop(provider);
}

bool YogaSMC::vsmcNotificationHandler(void *sensors, void *refCon, IOService *vsmc, IONotifier *notifier) {
    auto self = OSDynamicCast(YogaSMC, reinterpret_cast<OSMetaClassBase*>(sensors));
    if (sensors && vsmc) {
        DBGLOG("yogasmc", "got vsmc notification");
        auto &plugin = self->vsmcPlugin;
        auto ret = vsmc->callPlatformFunction(VirtualSMCAPI::SubmitPlugin, true, sensors, &plugin, nullptr, nullptr);
        if (ret == kIOReturnSuccess) {
            DBGLOG("yogasmc", "submitted plugin");
            return true;
        } else if (ret != kIOReturnUnsupported) {
            SYSLOG("yogasmc", "plugin submission failure %X", ret);
        } else {
            DBGLOG("yogasmc", "plugin submission to non vsmc");
        }
    } else {
        SYSLOG("yogasmc", "got null vsmc notification");
    }
    return false;
}

void YogaSMC::dispatchMessageGated(int* message, void* data)
{
    OSCollectionIterator* i = OSCollectionIterator::withCollection(_notificationServices);

    if (i) {
        while (IOService* service = OSDynamicCast(IOService, i->getNextObject())) {
            service->message(*message, this, data);
        }
        i->release();
    }
}

void YogaSMC::dispatchMessage(int message, void* data)
{
    if (_notificationServices->getCount() == 0) {
        IOLog("%s::%s No available notification consumer\n", getName(), name);
        return;
    }
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &YogaSMC::dispatchMessageGated), &message, data);
}

void YogaSMC::notificationHandlerGated(IOService *newService, IONotifier *notifier)
{
    if (notifier == _publishNotify) {
        IOLog("%s::%s Notification consumer published: %s\n", getName(), name, newService->getName());
        _notificationServices->setObject(newService);
    }

    if (notifier == _terminateNotify) {
        IOLog("%s::%s Notification consumer terminated: %s\n", getName(), name, newService->getName());
        _notificationServices->removeObject(newService);
    }
}

bool YogaSMC::notificationHandler(void *refCon, IOService *newService, IONotifier *notifier)
{
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &YogaSMC::notificationHandlerGated), newService, notifier);
    return true;
}

bool YogaSMC::findPNP(const char *id, IOACPIPlatformDevice **dev) {
    auto iterator = IORegistryIterator::iterateOver(gIOACPIPlane, kIORegistryIterateRecursively);
    if (!iterator) {
        IOLog("%s findPNP failed\n", getName());
        return nullptr;
    }
    auto pnp = OSString::withCString(id);

    while (auto entry = iterator->getNextObject()) {
        if (entry->compareName(pnp)) {
            *dev = OSDynamicCast(IOACPIPlatformDevice, entry);
            if (*dev) {
                IOLog("%s %s available at %s\n", getName(), id, (*dev)->getName());
                break;
            }
        }
    }
    iterator->release();
    pnp->release();

    return !!(*dev);
}

IOReturn YogaSMC::readECName(const char* name, UInt32 *result) {
    IOReturn ret = ec->evaluateInteger(name, result);
    switch (ret) {
        case kIOReturnSuccess:
            break;

        case kIOReturnBadArgument:
            IOLog("%s::%s read %s failed, bad argument (field size too large?)\n", getName(), name, name);
            break;
            
        default:
            IOLog("%s::%s read %s failed %x\n", getName(), name, name, ret);
            break;
    }
    return ret;
}

IOReturn YogaSMC::method_re1b(UInt32 offset, UInt32 *result) {
    OSObject* params[1] = {
        OSNumber::withNumber(offset, 32)
    };

    IOReturn ret = ec->evaluateInteger(readECOneByte, result, params, 1);
    if (ret != kIOReturnSuccess)
        IOLog("%s::%s read 0x%02x failed\n", getName(), name, offset);

    return ret;
}

IOReturn YogaSMC::method_recb(UInt32 offset, UInt32 size, UInt8 *data) {
    // Arg0 - offset in bytes from zero-based EC
    // Arg1 - size of buffer in bits
    OSObject* params[2] = {
        OSNumber::withNumber(offset, 32),
        OSNumber::withNumber(size * 8, 32)
    };
    OSObject* result;

    IOReturn ret = ec->evaluateObject(readECBytes, &result, params, 2);
    if (ret != kIOReturnSuccess) {
        IOLog("%s::%s read %d bytes @ 0x%02x failed\n", getName(), name, size, offset);
        return ret;
    }

    OSData* osdata = OSDynamicCast(OSData, result);
    if (!data) {
        IOLog("%s::%s read %d bytes @ 0x%02x invalid\n", getName(), name, size, offset);
        return kIOReturnNotReadable;
    }

    if (osdata->getLength() != size) {
        IOLog("%s::%s read %d bytes @ 0x%02x, got %d bytes\n", getName(), name, size, offset, osdata->getLength());
        return kIOReturnNoBandwidth;
    }

    memcpy(data, osdata->getBytesNoCopy(), size);

    result->release();
    return ret;
}

IOReturn YogaSMC::method_we1b(UInt32 offset, UInt32 value) {
    OSObject* params[2] = {
        OSNumber::withNumber(offset, 32),
        OSNumber::withNumber(value, 32)
    };
    UInt32 result;

    IOReturn ret = ec->evaluateInteger(writeECOneByte, &result, params, 2);
    if (ret != kIOReturnSuccess)
        IOLog("%s::%s write 0x%02x @ 0x%02x failed\n", getName(), name, value, offset);

    return ret;
}

void YogaSMC::dumpECOffset(UInt32 value) {
    UInt32 size = value >> 8;
    if (size) {
        UInt32 offset = value & 0xff;
        if (offset + size > 0x100) {
            IOLog("%s::%s read %d bytes @ 0x%02x exceeded\n", getName(), name, size, offset);
            return;
        }

        UInt8 *data = new UInt8[size];
        if (method_recb(offset, size, data) == kIOReturnSuccess) {
            int len = 0x10;
            char *buf = new char[len*3];
            if (size > 8) {
                IOLog("%s::%s %d bytes @ 0x%02x\n", getName(), name, size, offset);
                for (int i = 0; i < size; i += len) {
                    memset(buf, 0, len*3);
                    for (int j = 0; j < (len < size-i ? len : size-i); j++)
                        snprintf(buf+3*j, 4, "%02x ", data[i+j]);
                    buf[len*3-1] = '\0';
                    IOLog("%s::%s 0x%02x: %s", getName(), name, offset+i, buf);
                }
            } else {
                for (int j = 0; j < size; j++)
                    snprintf(buf+3*j, 4, "%02x ", data[j]);
                buf[size*3-1] = '\0';
                IOLog("%s::%s %d bytes @ 0x%02x: %s\n", getName(), name, size, offset, buf);
            }
            delete [] buf;
        }
        delete [] data;
    } else {
        UInt32 integer;
        if (method_re1b(value, &integer) == kIOReturnSuccess)
            IOLog("%s::%s 0x%02x: %02x\n", getName(), name, value, integer);
    }
}

void YogaSMC::setPropertiesGated(OSObject* props) {
    if (!ec) {
        IOLog("%s EC not available", getName());
        return;
    }

    OSDictionary* dict = OSDynamicCast(OSDictionary, props);
    if (!dict)
        return;

//    IOLog("%s::%s %d objects in properties\n", getName(), name, dict->getCount());
    OSCollectionIterator* i = OSCollectionIterator::withCollection(dict);

    if (i) {
        while (OSString* key = OSDynamicCast(OSString, i->getNextObject())) {
            if (key->isEqualTo("ReadECOffset")) {
                OSNumber * value = OSDynamicCast(OSNumber, dict->getObject("ReadECOffset"));
                if (value == nullptr) {
                    IOLog("%s::%s invalid number", getName(), name);
                    continue;
                }
                dumpECOffset(value->unsigned32BitValue());
            } else if (key->isEqualTo("ReadECName")) {
                OSString *value = OSDynamicCast(OSString, dict->getObject("ReadECName"));
                if (!value) {
                    IOLog("%s::%s invalid name", getName(), name);
                    continue;
                }
                if (value->getLength() !=4) {
                    IOLog("%s::%s invalid length %d", getName(), name, value->getLength());
                    continue;
                }
                UInt32 result;
                if (readECName(value->getCStringNoCopy(), &result) == kIOReturnSuccess)
                    IOLog("%s::%s %s: 0x%02x\n", getName(), name, value->getCStringNoCopy(), result);
            }
        }
        i->release();
    }

    return;
}

IOReturn YogaSMC::setProperties(OSObject *props) {
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &YogaSMC::setPropertiesGated), props);
    return kIOReturnSuccess;
}
