//
//  DYWMI.cpp
//  YogaSMC
//
//  Created by Zhen on 4/24/21.
//  Copyright © 2021 Zhen. All rights reserved.
//

#include "DYWMI.hpp"

extern "C"
{
    #include <UserNotification/KUNCUserNotifications.h>
}

OSDefineMetaClassAndStructors(DYWMI, YogaWMI);

YogaWMI* YogaWMI::withDY(IOService *provider) {
    WMI *candidate = new WMI(provider);
    candidate->initialize();

    YogaWMI* dev = nullptr;

    if (candidate->hasMethod(SENSOR_DATA_WMI_METHOD, 0))
        dev = OSTypeAlloc(DYWMI);
    else
        dev = OSTypeAlloc(YogaWMI);

    OSDictionary *dictionary = OSDictionary::withCapacity(1);

    dev->iname = provider->getName();
    dev->YWMI = candidate;

    if (!dev->init(dictionary)) {
        OSSafeReleaseNULL(dev);
        delete candidate;
    }

    dictionary->release();
    return dev;
}

IOReturn DYWMI::setProperties(OSObject *props) {
    commandGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &DYWMI::setPropertiesGated), props);
    return kIOReturnSuccess;
}

void DYWMI::setPropertiesGated(OSObject* props) {
    OSDictionary* dict = OSDynamicCast(OSDictionary, props);
    if (!dict)
        return;

    OSCollectionIterator* i = OSCollectionIterator::withCollection(dict);
    if (i) {
        while (OSString* key = OSDynamicCast(OSString, i->getNextObject())) {
            if (key->isEqualTo(updateSensorPrompt)) {
                OSNumber *value;
                getPropertyNumber(updateSensorPrompt);

                if (value->unsigned8BitValue() > sensorRange) {
                    AlwaysLog(valueMatched, updateSensorPrompt, sensorRange);
                    continue;
                }

                OSObject *result;
                if (getSensorInfo(value->unsigned8BitValue(), &result)) {
                    char sensorName[10];
                    snprintf(sensorName, 10, "Sensor %d", value->unsigned8BitValue());
                    if (result) {
                        setProperty(sensorName, result);
                        DebugLog(toggleSuccess, updateSensorPrompt, value->unsigned8BitValue(), "see ioreg");
                    } else {
                        AlwaysLog("%s empty result, please check the method return", sensorName);
                    }
                } else {
                    AlwaysLog(toggleFailure, updateSensorPrompt);
                }
                OSSafeReleaseNULL(result);
            } else {
                AlwaysLog("Unknown property %s", key->getCStringNoCopy());
            }
        }
        i->release();
    }

    return;
}

void DYWMI::processWMI() {
    setProperty("Feature", feature);
    sensorRange = YWMI->getInstanceCount(SENSOR_DATA_WMI_METHOD);
    registerService();
}

bool DYWMI::getSensorInfo(UInt8 index, OSObject **result) {
    bool ret;

    OSObject* params[1] = {
        OSNumber::withNumber(index, 32),
    };

    ret = YWMI->executeMethod(SENSOR_DATA_WMI_METHOD, result, params, 1, true);
    params[0]->release();

    if (!ret)
        AlwaysLog("Sensor %d evaluation failed", index);
    return ret;
}

const char* DYWMI::registerEvent(OSString *guid, UInt32 id) {
    if (guid->isEqualTo(SENSOR_EVENT_WMI_METHOD)) {
        sensorEvent = id;
        setProperty("Feature", "Sensor Event");
        return feature;
    }
    return nullptr;
}

void DYWMI::processBIOSEvent(OSObject *result) {
    OSArray *arr = OSDynamicCast(OSArray, result);
    if (!arr) {
        DebugLog("Unknown event");
        return;
    }

    OSString *name = OSDynamicCast(OSString, arr->getObject(0));
    if (!name) {
        DebugLog("Unknown event name");
        return;
    }

    OSString *desc = OSDynamicCast(OSString, arr->getObject(1));
    if (!desc) {
        DebugLog("Unknown event description");
        return;
    }

    OSNumber *severity = OSDynamicCast(OSNumber, arr->getObject(3));
    if (severity == nullptr) {
        DebugLog("Unknown event severity");
        return;
    }

    unsigned int flags = 0;
    switch (severity->unsigned8BitValue()) {
        case EVENT_SEVERITY_UNKNOWN:
            flags = kKUNCCautionAlertLevel;
            break;

        case EVENT_SEVERITY_OK:
            flags = kKUNCPlainAlertLevel | kKUNCNoDefaultButtonFlag;
            break;

        case EVENT_SEVERITY_WARNING:
            flags = kKUNCNoteAlertLevel;
            break;

        default:
            flags = kKUNCStopAlertLevel;
            break;
    }
#ifndef DEBUG
    if (flags < kKUNCPlainAlertLevel) {
#endif
        DebugLog("BIOS Event: %s - %s - %d", name->getCStringNoCopy(), desc->getCStringNoCopy(), severity->unsigned8BitValue());
        char header[20];
        strncpy(header, name->getCStringNoCopy(), name->getLength() < 20 ? name->getLength() : 20);
        char message[50];
        strncpy(message, desc->getCStringNoCopy(), desc->getLength() < 50 ? desc->getLength() : 50);
        kern_return_t notificationError;
        notificationError = KUNCUserNotificationDisplayNotice(flags < kKUNCPlainAlertLevel ? 0 : 2,
                                                              flags,
                                                              (char *) "",
                                                              (char *) "",
                                                              (char *) "",
                                                              header,
                                                              message,
                                                              (char *) "OK");
        if (notificationError != kIOReturnSuccess)
            AlwaysLog("Failed to send BIOS Event");
#ifndef DEBUG
    }
#endif
}

void DYWMI::ACPIEvent(UInt32 argument) {
    if (argument != sensorEvent) {
        super::ACPIEvent(argument);
        return;
    }

    OSObject *result;
    if (!YWMI->getEventData(argument, &result))
        AlwaysLog("message: Unknown ACPI notification 0x%04X", argument);
    else
        processBIOSEvent(result);
    OSSafeReleaseNULL(result);
}
