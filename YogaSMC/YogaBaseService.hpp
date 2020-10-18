//
//  YogaBaseService.hpp
//  YogaSMC
//
//  Created by Zhen on 10/17/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef YogaBaseService_hpp
#define YogaBaseService_hpp

#include <IOKit/IOCommandGate.h>
#include <IOKit/IOService.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include "common.h"
#include "message.h"
//#include "YogaSMCUserClientPrivate.hpp"

class YogaBaseService : public IOService {
    typedef IOService super;
    OSDeclareDefaultStructors(YogaBaseService);

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

    /**
     *  UserClient
     */
//    YogaSMCUserClient *client {nullptr};

    void dispatchMessage(int message, void* data);

    /**
     *  Iterate over IOACPIPlane for PNP device
     *
     *  @param id PNP name
     *  @param dev target ACPI device
     *  @return true if VPC is available
     */
    bool findPNP(const char *id, IOACPIPlatformDevice **dev);

public:
    virtual bool init(OSDictionary *dictionary) APPLE_KEXT_OVERRIDE;
    virtual IOService *probe(IOService *provider, SInt32 *score) APPLE_KEXT_OVERRIDE;

    virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
    virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;

//    friend class YogaSMCUserClient;
};
#endif /* YogaBaseService_hpp */
