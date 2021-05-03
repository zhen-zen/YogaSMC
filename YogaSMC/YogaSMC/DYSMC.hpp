//
//  DYSMC.hpp
//  YogaSMC
//
//  Created by Zhen on 4/28/21.
//  Copyright © 2021 Zhen. All rights reserved.
//

#ifndef DYSMC_hpp
#define DYSMC_hpp

#include "DYWMI.hpp"
#include "YogaSMC.hpp"

#define SENSOR_TYPE_TEMPERATURE 2
#define SENSOR_TYPE_AIR_FLOW    12

/**
 * Sensor Package Definition
 *
 * 0x1: Name
 * 0x2: Description
 * 0x3: SensorType
 *   0: Unknown
 *   1: Other
 *   2: Temperature
 *   5: Tachometer
 *  11: Presence
 *  12: Air Flow
 * 0x4: OtherSensorType
 * 0x5: OperationalStatus
 *   0: Unknown
 *   1: Other
 *   2: OK
 *   3: Degraded
 *   5: Predictive Failure
 *   6: Error
 *  10: Stopped
 *  12: No Contact
 * 0x6: Size
 * 0x7: PossibleStates[Size]
 * 0x8: CurrentState
 *
 * 0x9: BaseUnits
 *   1: Other
 *   2: Degrees C
 *   3: RPM
 * 0xa: UnitModifier (sint32)
 * 0xb: CurrentReading
 * 0xc: RateUnits
 *   0: None
 *   1: Per Microsecond
 *   2: Per Millisecond
 *   3: Per Second
 *   4: Per Minute
 *   5: Per Hour
 *   6: Per Day
 *   7: Per Week
 *   8: Per Month
 *   9: Per Year
 */

class DYSMC : public YogaSMC
{
    typedef YogaSMC super;
    OSDeclareDefaultStructors(DYSMC)

private:
    /**
     *  WMI device, in place of provider and direct ACPI evaluations
     */
    DYWMI *wmis {nullptr};

    /**
     *  Availble sensor range could be 0..InstanceCount or 0..InstanceCount-1
     */
    UInt8 sensorRange {0};

    /**
     *  Corresponding sensor index
     */
    UInt8 sensorIndex[MAX_SENSOR];

    bool addTachometerKey(OSString *name);
    bool addTemperatureKey(OSString *name);

    void addVSMCKey() APPLE_KEXT_OVERRIDE;
    void updateEC() APPLE_KEXT_OVERRIDE;
    virtual inline IOTimerEventSource *initPoller() APPLE_KEXT_OVERRIDE {
        return IOTimerEventSource::timerEventSource(this, [](OSObject *object, IOTimerEventSource *sender) {
            auto smc = OSDynamicCast(DYSMC, object);
            if (smc) smc->updateEC();
        });
    };

public:
    static DYSMC *withDevice(IOService *provider, IOACPIPlatformDevice *device);

    void setWMI(IOService* instance);
};

#endif /* DYSMC_hpp */
