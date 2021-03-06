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

YogaWMI* YogaWMI::withDYWMI(WMI *provider) {
    YogaWMI* dev = nullptr;

    if (provider->hasMethod(SENSOR_DATA_WMI_ARRAY, 0))
        dev = OSTypeAlloc(DYWMI);
    else
        dev = OSTypeAlloc(YogaWMI);

    OSDictionary *dictionary = OSDictionary::withCapacity(1);

    dev->iname = provider->getName();
    dev->YWMI = provider;

    if (!dev->init(dictionary))
        OSSafeReleaseNULL(dev);

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

                if (sensorRange <= value->unsigned8BitValue()) {
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
    sensorRange = YWMI->getInstanceCount(SENSOR_DATA_WMI_ARRAY);
    registerService();
}

bool DYWMI::getSensorInfo(UInt8 index, OSObject **result) {
    IOReturn ret;

    ret = YWMI->queryBlock(SENSOR_DATA_WMI_ARRAY, index, result, true);
    if (ret != kIOReturnSuccess)
        AlwaysLog("Sensor %d evaluation failed", index);

    return (ret == kIOReturnSuccess);
}

const char* DYWMI::registerEvent(OSString *guid, UInt32 id) {
    if (guid->isEqualTo(SENSOR_EVENT_WMI_METHOD)) {
        sensorEvent = id;
        setProperty("Sensor Event", true);
        return feature;
    }
    return nullptr;
}

bool DisplayUserNotification(OSString *name, OSString *desc, unsigned int flags) {
    kern_return_t notificationError;
    char header[20];
    strlcpy(header, name->getCStringNoCopy(), sizeof(header));
    char message[50];
    strlcpy(message, desc->getCStringNoCopy(), sizeof(message));
    notificationError = KUNCUserNotificationDisplayNotice(flags < kKUNCPlainAlertLevel ? 0 : 2 ,
                                                          flags,
                                                          (char *) "",
                                                          (char *) "",
                                                          (char *) "",
                                                          header,
                                                          message,
                                                          (char *) "OK");
    return (notificationError == kIOReturnSuccess);
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
            flags = kKUNCPlainAlertLevel;
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
        AlwaysLog("BIOS Event: %s - %s - %d", name->getCStringNoCopy(), desc->getCStringNoCopy(), severity->unsigned8BitValue());
        if (!DisplayUserNotification(name, desc, flags))
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
