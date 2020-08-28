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
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <VirtualSMCSDK/kern_vsmcapi.hpp>
#include "KeyImplementations.hpp"
#include "common.h"
#include "message.h"

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

protected:
    const char* name;

    IOWorkLoop *workLoop {nullptr};
    IOCommandGate *commandGate {nullptr};

    void dispatchMessage(int message, void* data);

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
     *  Sensors configuration
     */
    OSDictionary* sensors;

    
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

    friend BDVT;
};

#endif /* YogaSMC_hpp */
