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

    /**
     *  EC device
     */
    IOACPIPlatformDevice *ec {nullptr};

    /**
     *  Iterate over IOACPIPlane for PNP device
     *
     *  @param id PNP name
     *  @param dev target ACPI device
     *  @return true on sucess
     */
    bool findPNP(const char *id, IOACPIPlatformDevice **dev);

    /**
     *  Related ACPI methods
     */
    static constexpr const char *readECOneByte      = "RE1B";
    static constexpr const char *readECBytes        = "RECB";
    static constexpr const char *writeECOneByte     = "WE1B";
    static constexpr const char *writeECBytes       = "WECB";

    /**
     *  Dump desired EC field
     *
     *  @param value = offset | size << 8
     */
    void dumpECOffset(UInt32 value);

protected:
    IOWorkLoop *workLoop {nullptr};
    IOCommandGate *commandGate {nullptr};

    void dispatchMessage(int message, void* data);

    /**
     *  Get pnp id of VPC device
     *
     *  @return nullptr if vpc is not configured
     */
    inline virtual const char *getVPCName() {return nullptr;};

    /**
     *  Wrapper for RE1B
     *
     *  @param offset EC field offset
     *  @param result EC field value
     *
     *  @return kIOReturnSuccess on success
     */
    IOReturn method_re1b(UInt32 offset, UInt32 *result);

    /**
     *  Wrapper for RECB
     *
     *  @param offset EC field offset
     *  @param size EC field length in bytes
     *  @param data EC field value
     *  @return kIOReturnSuccess on success
     */
    IOReturn method_recb(UInt32 offset, UInt32 size, UInt8 *data);

    /**
     *  Wrapper for WE1B
     *
     *  @param offset EC field offset
     *  @param value EC field value
     *
     *  @return kIOReturnSuccess on success
     */
    IOReturn method_we1b(UInt32 offset, UInt32 result);

    /**
     *  Read custom field
     *
     *  @param name EC field name
     *  @param result EC field value
     *
     *  @return kIOReturnSuccess on success
     */
    IOReturn readECName(const char* name, UInt32 *result);

    virtual void setPropertiesGated(OSObject* props);

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

public:
    virtual bool init(OSDictionary *dictionary) APPLE_KEXT_OVERRIDE;
    virtual IOService *probe(IOService *provider, SInt32 *score) APPLE_KEXT_OVERRIDE;

    virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
    virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;

    virtual IOReturn setProperties(OSObject* props) APPLE_KEXT_OVERRIDE;

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
