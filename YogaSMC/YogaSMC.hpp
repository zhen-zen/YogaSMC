//  SPDX-License-Identifier: GPL-2.0-only
//
//  YogaSMC.hpp
//  YogaSMC
//
//  Created by Zhen on 2020/5/21.
//  Copyright © 2020 Zhen. All rights reserved.
//
//#include "kern_util.hpp"
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOService.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include "WMI.h"

#define kIOACPIMessageD0 0xd0
#define kIOACPIMessageReserved 0x80

#define YMC_WMI_METHOD "09b0ee6e-c3fd-4243-8da1-7911ff80bb8c"
#define YMC_WMI_EVENT  "06129d99-6083-4164-81ad-f092f9d773a6"

#define kDeliverNotifications   "RM,deliverNotifications"

enum
{
    // from keyboard to mouse/touchpad
    kSMC_setDisableTouchpad = iokit_vendor_specific_msg(100),   // set disable/enable touchpad (data is bool*)
    kSMC_getDisableTouchpad = iokit_vendor_specific_msg(101),   // get disable/enable touchpad (data is bool*)

    // from sensor to keyboard
    kSMC_setKeyboardStatus  = iokit_vendor_specific_msg(200),   // set disable/enable keyboard (data is bool*)
    kSMC_getKeyboardStatus  = iokit_vendor_specific_msg(201)    // get disable/enable keyboard (data is bool*)
};

enum
{
    kYogaMode_laptop = 1,   // 0-90 degree
    kYogaMode_tablet = 2,   // 0/360 degree
    kYogaMode_stand  = 3,   // 180-360 degree, ∠ , screen face up
    kYogaMode_tent   = 4    // 180-360 degree, ∧ , screen upside down, trigger rotation?
} kYogaMode;

class YogaWMI : public IOService
{
    typedef IOService super;
    OSDeclareDefaultStructors(YogaWMI)

private:
//    IOACPIPlatformDevice *device {nullptr};
    WMI* YWMI {nullptr};

    void dispatchMessage(int message, void* data);
    void dispatchMessageGated(int* message, void* data);

    bool notificationHandler(void * refCon, IOService * newService, IONotifier * notifier);
    void notificationHandlerGated(IOService * newService, IONotifier * notifier);

    IONotifier* _publishNotify {nullptr};
    IONotifier* _terminateNotify {nullptr};
    OSSet* _notificationServices {nullptr};
    const OSSymbol* _deliverNotification {nullptr};

    bool isYMC {false};

protected:
    IOWorkLoop *workLoop {nullptr};
    IOCommandGate *commandGate {nullptr};

    /**
     *  VPC device
     */
    IOACPIPlatformDevice *vpc {nullptr};

    /**
     *  VPC event notifiers
     */
    IONotifier *VPCNotifiers;

    /**
     *  Handle notification about VPC0
     *
     *  @param messageType      kIOACPIMessageDeviceNotification
     *  @param provider         The ACPI device being connected or disconnected
     *  @param messageArgument  nullptr
     *
     *  @return kIOReturnSuccess
     */
    static IOReturn VPCNotification(void *target, void *refCon, UInt32 messageType, IOService *provider, void *messageArgument, vm_size_t argSize);

    OSDictionary *Event {nullptr};

    /**
     *  Find notify id and other properties of an event
     *
     *  @param key  name of event dictionary
     */
    void getNotifyID(OSString *key);

    /**
     *  Iterate through IOACPIPlane for VPC
     *
     *  @return true if VPC is available
     */
    bool findVPC();

    /**
     *  Get VPC config
     *
     *  @return true if VPC is initialized successfully
     */
    inline virtual bool initVPC() {return true;};
    
    /**
     *  Update VPC EC status
     */
    inline virtual void updateVPC() {return;};

    /**
     *  Get pnp id of VPC device
     *
     *  @return nullptr if vpc is not configured
     */
    inline virtual OSString *getPnp() {return nullptr;};

    /**
     *  Current Yoga Mode, see kYogaMode
     */
    int YogaMode {kYogaMode_laptop};

    /**
     *  Corresponding event to trigger after receiving a message
     *
     *  @param argument  argument of message
     */
    virtual void YogaEvent(UInt32 argument);

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
    virtual bool init(OSDictionary *dictionary) override;
    virtual void free(void) override;
    virtual IOService *probe(IOService *provider, SInt32 *score) override;

    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;

    virtual IOReturn message(UInt32 type, IOService *provider, void *argument) override;
};
