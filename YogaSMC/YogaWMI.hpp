//  SPDX-License-Identifier: GPL-2.0-only
//
//  YogaWMI.hpp
//  YogaSMC
//
//  Created by Zhen on 2020/5/21.
//  Copyright © 2020 Zhen. All rights reserved.
//
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOService.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include "message.h"
#include "WMI.h"

class YogaWMI : public IOService
{
    typedef IOService super;
    OSDeclareDefaultStructors(YogaWMI)

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
     *  WMI device, in place of provider and direct ACPI evaluations
     */
    WMI* YWMI {nullptr};

    void dispatchMessage(int message, void* data);

    /**
     *  Find notify id and other properties of an event
     *
     *  @param key  name of event dictionary
     */
    void getNotifyID(OSString *key);

    /**
     *  Iterate over IOACPIPlane for PNP device
     *
     *  @param id PNP name
     *  @param dev target ACPI device
     *  @return true if VPC is available
     */
    bool findPNP(const char *id, IOACPIPlatformDevice **dev);

    /**
     *  Get pnp id of VPC device
     *
     *  @return nullptr if not specified
     */
    inline virtual const char *getVPCName() {return nullptr;};

    /**
     *  Vendor specific WMI analyze
     */
    inline virtual void processWMI() {};

    OSDictionary *Event {nullptr};

    /**
     *  Check WMI event id
     *
     *  @param cname WMI event name
     *  @param id  WMI event id
     */
    virtual void checkEvent(const char *cname, UInt32 id);
    
    /**
     *  Corresponding event to trigger after receiving a message
     *
     *  @param argument  argument of message
     */
    virtual void ACPIEvent(UInt32 argument);

    /**
     *  Current Keyboard status
     */
    bool Keyboardenabled {true};
    
    /**
     *  Current TouchPad status
     */
    bool TouchPadenabled {true};

    /**
     *  Switch  touchpad status
     */
    void toggleTouchpad();
    
    /**
     *  Switch keyboard status
     */
    void toggleKeyboard();

    /**
     *  Set keyboard and touchpad status
     *
     *  @param enable  desired status
     */
    void setTopCase(bool enable);

    /**
     *  Update keyboard and touchpad status
     *
     *  @return false if keyboard and touchpad status mismatch
     */
    bool updateTopCase();

public:
    virtual bool init(OSDictionary *dictionary) APPLE_KEXT_OVERRIDE;
    virtual IOService *probe(IOService *provider, SInt32 *score) APPLE_KEXT_OVERRIDE;

    virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
    virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;

    virtual IOReturn message(UInt32 type, IOService *provider, void *argument) APPLE_KEXT_OVERRIDE;
    virtual IOReturn setPowerState(unsigned long powerState, IOService * whatDevice) APPLE_KEXT_OVERRIDE;
};
