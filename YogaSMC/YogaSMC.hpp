//
//  YogaSMC.hpp
//  YogaSMC
//
//  Created by Zhen on 7/29/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef YogaSMC_hpp
#define YogaSMC_hpp

#include <VirtualSMCSDK/kern_vsmcapi.hpp>
#include "KeyImplementations.hpp"
#include "YogaBaseService.hpp"

#define MAX_SENSOR 0x10
#define POLLING_INTERVAL 2000

class YogaSMC : public YogaBaseService
{
    typedef YogaBaseService super;
    OSDeclareDefaultStructors(YogaSMC)

protected:
    bool awake {false};
    bool ready {false};

    IOTimerEventSource* poller {nullptr};

    /**
     *  VirtualSMC service registration notifier
     */
    IONotifier *vsmcNotifier {nullptr};

    /**
     *  Registered plugin instance
     */
    VirtualSMCAPI::Plugin vsmcPlugin {
        xStringify(PRODUCT_NAME),
        parseModuleVersion(xStringify(MODULE_VERSION)),
        VirtualSMCAPI::Version,
    };

    /**
     *  Sensors configuration
     */
    OSDictionary* conf {nullptr};

    /**
     *  Add available SMC keys
     */
    virtual void addVSMCKey();

    /**
     *  ACPI method for sensors
     */
    const char *sensorMethods[MAX_SENSOR];

    /**
     *  Enabled sensor count
     */
    UInt8 sensorCount {0};

    /**
     *  EC sensor base
     */
    UInt8 ECSensorBase {0};

    /**
     *  Current sensor reading obtained from ACPI
     */
    _Atomic(uint32_t) currentSensor[MAX_SENSOR];

    /**
     *  Poll EC field for sensor data
     */
    void updateEC();
    virtual inline void updateECVendor() {};

    virtual void setPropertiesGated(OSObject* props);

public:
    virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
    virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;

    virtual IOReturn setPowerState(unsigned long powerStateOrdinal, IOService * whatDevice) APPLE_KEXT_OVERRIDE;
    virtual IOReturn setProperties(OSObject* props) APPLE_KEXT_OVERRIDE;

    static YogaSMC *withDevice(IOService *provider, IOACPIPlatformDevice *device);

    /**
     *  Submit the keys to received VirtualSMC service.
     *
     *  @param sensors   SMCBatteryManager service
     *  @param refCon    reference
     *  @param vsmc      VirtualSMC service
     *  @param notifier  created notifier
     */
    static bool vsmcNotificationHandler(void *sensors, void *refCon, IOService *vsmc, IONotifier *notifier);
};

#endif /* YogaSMC_hpp */
