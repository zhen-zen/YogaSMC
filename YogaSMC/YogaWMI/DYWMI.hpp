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
#ifndef ALTER
#include "DYSMC.hpp"
#endif

#define SENSOR_DATA_WMI_METHOD  "8f1f6435-9f42-42c8-badc-0e9424f20c9a"

class DYWMI : public YogaWMI
{
    typedef YogaWMI super;
    OSDeclareDefaultStructors(DYWMI)

private:
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

    virtual void setPropertiesGated(OSObject* props);

#ifndef ALTER
    friend void DYSMC::setWMI(IOService *instance);
#endif

    void processWMI() APPLE_KEXT_OVERRIDE;
//    void ACPIEvent(UInt32 argument) APPLE_KEXT_OVERRIDE;
//    void checkEvent(const char *cname, UInt32 id) APPLE_KEXT_OVERRIDE;

public:
    static DYWMI *withDevice(IOService *provider);

    virtual IOReturn setProperties(OSObject* props) APPLE_KEXT_OVERRIDE;
};
#endif /* DYWMI_hpp */
