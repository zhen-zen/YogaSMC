//
//  DYSMC.cpp
//  YogaSMC
//
//  Created by Zhen on 4/28/21.
//  Copyright © 2021 Zhen. All rights reserved.
//

#include "DYSMC.hpp"
#include <Headers/kern_devinfo.hpp>

OSDefineMetaClassAndStructors(DYSMC, YogaSMC);

static const struct sensorPair DYPresetTachometer[] = {
    {KeyF0Ac(0), "CPU Fan"},
    {KeyF0Ac(0), "CPU Fan Speed"},
    {KeyF0Ac(1), "GPU Fan"},
    {KeyF0Ac(1), "Rear Chassis Fan"},
    {KeyF0Ac(1), "Rear Chassis Fan Speed"},
    {KeyF0Ac(2), "Power Supply Fan"},
    {KeyF0Ac(2), "Power Supply Fan Speed"},
    {KeyF0Ac(3), "Front Chassis Fan"}
};

static const struct sensorPair DYPresetTemperature[] = {
    {KeyTB0T(1), "Battery Temperature"},
    {KeyTCGC,    "Discrete Graphics Temperature"},
    {KeyTCGC,    "GPU Temperature"},
    {KeyTCSA,    "Private temp1 (CPU VR Temp)"},
    {KeyTCXC,    "CPU Temperature"},
    {KeyTG0P(0), "GPU0 Temperature"},
    {KeyTG0P(1), "GPU1 Temperature"},
    {KeyTH0a,    "M.2 Temperature"},
    {KeyTH0a,    "M.2 0 Temperature"},
    {KeyTH0b,    "M.2 1 Temperature"},
    {KeyTH0b,    "HDD Temperature"},
    {KeyTM0P,    "Private temp2 (Memory Temp)"},
    {KeyTPCD,    "Local Temperature"},
    {KeyTTRD,    "Charger Temperature"},
    {KeyTW0P,    "Chassis Temperature"},
    {KeyTW0P,    "U518"},
    {KeyTh0H(1), "System Ambient Temperature"},
    {KeyTh0H(2), "System Ambient1 Temperature"},
    {KeyTs0P(0), "Remote Temperature"}
};

bool DYSMC::addTachometerKey(OSString *name) {
    for (auto &pair : DYPresetTachometer) {
        if (name->isEqualTo(pair.name)) {
            VirtualSMCAPI::addKey(pair.key, vsmcPlugin.data, VirtualSMCAPI::valueWithFp(0, SmcKeyTypeFpe2, new atomicFpKey(&currentSensor[sensorCount])));
            return true;
        }
    }
    return false;
}

bool DYSMC::addTemperatureKey(OSString *name) {
    for (auto &pair : DYPresetTemperature) {
        if (name->isEqualTo(pair.name)) {
            VirtualSMCAPI::addKey(pair.key, vsmcPlugin.data, VirtualSMCAPI::valueWithSp(0, SmcKeyTypeSp78, new atomicSpKey(&currentSensor[sensorCount])));
            return true;
        }
    }
    return false;
}

void DYSMC::addVSMCKey() {
    if (!wmis) {
        super::addVSMCKey();
        return;
    }

    OSObject *result = nullptr;

    OSDictionary *enabled = OSDictionary::withCapacity(wmis->sensorRange);
    OSDictionary *disabled = OSDictionary::withCapacity(wmis->sensorRange);

    UInt8 fanCount = 0;

    for (UInt8 index = 0; index < wmis->sensorRange && sensorCount < MAX_SENSOR; ++index) {
        OSSafeReleaseNULL(result);

        if (!wmis->getSensorInfo(index, &result)) {
            AlwaysLog("Failed to evaluate sensor %d", index);
            continue;
        }

        OSArray *arr = OSDynamicCast(OSArray, result);
        if (!arr) {
            AlwaysLog("Sensor %d returns empty result, please check if the method has valid return", index);
            continue;
        }

        OSString *sensorName = OSDynamicCast(OSString, arr->getObject(0));
        if (!sensorName || sensorName->isEqualTo("")) {
            char tempName[10];
            snprintf(tempName, 10, "Sensor %d", index);
            disabled->setObject(tempName, arr);
            AlwaysLog("Failed to get name for sensor %d", index);
            continue;
        }

        if (sensorName->isEqualTo("Unknown Sensor")) {
            AlwaysLog("Unknown Sensor %d", index);
            continue;
        }

        OSNumber *statesSize = OSDynamicCast(OSNumber, arr->getObject(5));
        if (statesSize == nullptr) {
            disabled->setObject(sensorName, arr);
            AlwaysLog("Failed to get statesSize for %s", sensorName->getCStringNoCopy());
            continue;
        }

        OSString *state = OSDynamicCast(OSString, arr->getObject(6 + statesSize->unsigned32BitValue()));
        if (!state || state->isEqualTo("Not Present")) {
            disabled->setObject(sensorName, arr);
            AlwaysLog("State for %s: %s", sensorName->getCStringNoCopy(), state ? state->getCStringNoCopy() : "Unavailable");
            continue;
        }

        OSNumber *reading = OSDynamicCast(OSNumber, arr->getObject(9 + statesSize->unsigned32BitValue()));
        if (reading == nullptr) {
            disabled->setObject(sensorName, arr);
            AlwaysLog("Failed to get value for %s", sensorName->getCStringNoCopy());
            continue;
        }

        OSNumber *type = OSDynamicCast(OSNumber, arr->getObject(2));
        if (type == nullptr) {
            AlwaysLog("Failed to get type for %s", sensorName->getCStringNoCopy());
            continue;
        }

        switch (type->unsigned32BitValue()) {
            case SENSOR_TYPE_TEMPERATURE:
                if (reading->unsigned32BitValue() == 0) {
                    disabled->setObject(sensorName, arr);
                    AlwaysLog("Temperature is 0 for %s", sensorName->getCStringNoCopy());
                    continue;
                }
                if (!addTemperatureKey(sensorName)) {
                    disabled->setObject(sensorName, arr);
                    AlwaysLog("Unknown temperature %s", sensorName->getCStringNoCopy());
                    continue;
                }
                break;

            case SENSOR_TYPE_AIR_FLOW:
                if (!addTachometerKey(sensorName)) {
                    disabled->setObject(sensorName, arr);
                    AlwaysLog("Unknown tachometer %s", sensorName->getCStringNoCopy());
                    continue;
                }
                fanCount++;
                break;

            default:
                disabled->setObject(sensorName, arr);
                AlwaysLog("Unknown sensor %s", sensorName->getCStringNoCopy());
                continue;
        }

        sensorIndex[sensorCount++] = index;
        enabled->setObject(sensorName, arr);
    }
    OSSafeReleaseNULL(result);

    if (fanCount)
        VirtualSMCAPI::addKey(KeyFNum, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(fanCount, nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));

    setProperty("Sensors", enabled);
    setProperty("Disabled Sensors", disabled);
    OSSafeReleaseNULL(enabled);
    OSSafeReleaseNULL(disabled);

    super::addVSMCKey();
}

void DYSMC::updateECVendor() {
    OSObject *result = nullptr;

    for (UInt8 index = 0; index < ECSensorBase; ++index) {
        OSSafeReleaseNULL(result);
        if (!wmis->getSensorInfo(sensorIndex[index], &result)) {
            AlwaysLog("Failed to evaluate sensor %d", sensorIndex[index]);
            continue;
        }

        OSArray *arr = OSDynamicCast(OSArray, result);
        if (!arr) {
            AlwaysLog("Sensor %d returns empty result, please check if the method has valid return", sensorIndex[index]);
            continue;
        }

        OSNumber *statesSize = OSDynamicCast(OSNumber, arr->getObject(5));
        if (statesSize == nullptr) {
            AlwaysLog("Failed to get statesSize for Sensor %d", sensorIndex[index]);
            continue;
        }

        OSNumber *modifier = OSDynamicCast(OSNumber, arr->getObject(8 + statesSize->unsigned32BitValue()));
        if (modifier == nullptr) {
            AlwaysLog("Failed to get modifier for sensor %d", sensorIndex[index]);
            continue;
        }

        OSNumber *reading = OSDynamicCast(OSNumber, arr->getObject(9 + statesSize->unsigned32BitValue()));
        if (reading == nullptr) {
            AlwaysLog("Failed to get reading for sensor %d", sensorIndex[index]);
            continue;
        }

        SInt32 value = reading->unsigned32BitValue() + (SInt32)(modifier->unsigned32BitValue());
        atomic_store_explicit(&currentSensor[index], value, memory_order_release);
    }
    OSSafeReleaseNULL(result);
}

void DYSMC::getWMISensor(IOService *provider) {
    wmis = OSDynamicCast(DYWMI, provider->getProperty("DYSensor"));
}
