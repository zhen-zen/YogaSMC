//
//  DYWMI.hpp
//  YogaSMC
//
//  Created by Zhen on 4/24/21.
//  Copyright © 2021 Zhen. All rights reserved.
//

#ifndef DYWMI_hpp
#define DYWMI_hpp

#include "YogaWMI.hpp"

#define SENSOR_DATA_WMI_ARRAY   "8f1f6435-9f42-42c8-badc-0e9424f20c9a"
#define SENSOR_EVENT_WMI_METHOD "2b814318-4be8-4707-9d84-a190a859b5d0"

#define kIOACPIMessageDYSensor  0xd0

#define EVENT_SEVERITY_UNKNOWN  0
#define EVENT_SEVERITY_OK       5
#define EVENT_SEVERITY_WARNING  10

/**
 * BIOS Event (index start from 1)
 *
 * 0x1: Name
 * 0x2: Description
 * 0x3: Category
 *   0: Unknown
 *   1: Configuration Change
 *   2: Button Pressed
 *   3: Sensor
 *   4: BIOS Settings
 * 0x4: Severity
 *   0: Unknown
 *   5: OK
 *  10: Degraded/Warning
 *  15: Minor Failure
 *  20: Major Failure
 *  25: Critical Failure
 *  30: Non-recoverable Error
 * 0x5: Status
 *   0: Unknown
 *   1: Other
 *   2: OK
 *   4: Stressed
 *   5: Predictive Failure
 *  10: Stopped
 *  12: Aborted
 */

class DYWMI : public YogaWMI
{
    typedef YogaWMI super;
    OSDeclareDefaultStructors(DYWMI)

private:
    static constexpr const char *feature = "Sensor";
    UInt32 sensorEvent {0xa0};

    void processBIOSEvent(OSObject *result);

    UInt8 sensorRange {0};

    /**
     *  Parse Sensor Info
     *
     *  @param index sensor info to be executed
     *  @param result sensor info
     *
     *  @return true if success
     */
    bool getSensorInfo (UInt8 index, OSObject **result);

    void setPropertiesGated(OSObject* props);

#ifndef ALTER
    friend class DYSMC;
#endif

    void processWMI() APPLE_KEXT_OVERRIDE;
    void ACPIEvent(UInt32 argument) APPLE_KEXT_OVERRIDE;
    const char* registerEvent(OSString *guid, UInt32 id) APPLE_KEXT_OVERRIDE;

public:
    virtual IOReturn setProperties(OSObject* props) APPLE_KEXT_OVERRIDE;
};
#endif /* DYWMI_hpp */
