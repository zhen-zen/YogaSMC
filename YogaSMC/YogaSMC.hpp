//
//  YogaSMC.hpp
//  YogaSMC
//
//  Created by Zhen on 7/29/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef YogaSMC_hpp
#define YogaSMC_hpp

#include <IOKit/IOCommandGate.h>
#include <IOKit/IOService.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <VirtualSMCSDK/kern_vsmcapi.hpp>
#include "KeyImplementations.hpp"
#include "common.h"
#include "message.h"

#define MAX_SENSOR 0x10
#define POLLING_INTERVAL 2000

class YogaSMC : public IOService
{
    typedef IOService super;
    OSDeclareDefaultStructors(YogaSMC)

private:
    void dispatchMessageGated(int* message, void* data);

    bool notificationHandler(void * refCon, IOService * newService, IONotifier * notifier);
    void notificationHandlerGated(IOService * newService, IONotifier * notifier);

    IONotifier* _publishNotify {nullptr};
    IONotifier* _terminateNotify {nullptr};
    OSSet* _notificationServices {nullptr};
    const OSSymbol* _deliverNotification {nullptr};

    /**
     *  Current sensor reading obtained from ACPI
     */
    _Atomic(uint32_t) currentSensor[MAX_SENSOR];
     

protected:
    const char* name;

    IOWorkLoop *workLoop {nullptr};
    IOCommandGate *commandGate {nullptr};
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
     *  EC device
     */
    IOACPIPlatformDevice *ec {nullptr};

    /**
     *  Add available SMC keys
     */
    virtual void addVSMCKey();

    /**
     *  ACPI method for sensors
     */
    const char *sensorMethod[MAX_SENSOR];

    /**
     *  Enabled sensor count
     */
    UInt sensorCount {0};

    /**
     *  Poll EC field for sensor data
     */
    void updateEC();

public:
    virtual bool init(OSDictionary *dictionary) APPLE_KEXT_OVERRIDE;

    virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
    virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;

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

    void dispatchMessage(int message, void* data);

    /**
     *  Sensors configuration
     */
    OSDictionary* conf {nullptr};
};

#endif /* YogaSMC_hpp */
