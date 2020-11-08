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

    /**
     *  Iterate over IOACPIPlane for PNP device
     *
     *  @param id PNP name
     *  @param dev target ACPI device
     *  @return true if VPC is available
     */
    bool findPNP(const char *id, IOACPIPlatformDevice **dev);

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

    /**
     *  Enable power management support
     */
    inline virtual bool PMSupport() {return false;};

    /**
     *  Related ACPI methods
     */
    static constexpr const char *readECOneByte      = "RE1B";
    static constexpr const char *readECBytes        = "RECB";
    static constexpr const char *writeECOneByte     = "WE1B";
    static constexpr const char *writeECBytes       = "WECB";

    /**
     *  EC device
     */
    IOACPIPlatformDevice *ec {nullptr};

    /**
     *  Test EC capability
     */
    void validateEC();
    
    /**
     *  EC access capability, will be update on init
     *  BIT 0 Read
     *  BIT 1 Write
     */
    UInt8 ECAccessCap {0};

    /**
     *  Wrapper for RE1B
     *
     *  @param offset EC field offset
     *  @param result EC field value
     *
     *  @return kIOReturnSuccess on success
     */
    IOReturn method_re1b(UInt32 offset, UInt8 *result);

    /**
     *  Wrapper for RECB
     *
     *  @param offset EC field offset
     *  @param size EC field length in bytes
     *  @param data EC field value
     *
     *  @return kIOReturnSuccess on success
     */
    IOReturn method_recb(UInt32 offset, UInt32 size, OSData **data);

    /**
     *  Wrapper for WE1B
     *
     *  @param offset EC field offset
     *  @param value EC field value
     *
     *  @return kIOReturnSuccess on success
     */
    IOReturn method_we1b(UInt32 offset, UInt8 value);

    /**
     *  Read custom field
     *
     *  @param name EC field name
     *  @param result EC field value
     *
     *  @return kIOReturnSuccess on success
     */
    IOReturn readECName(const char* name, UInt32 *result);

    /**
     *  Send key event through VoodooPS2
     * @param keyCode event
     * @param goingDown pressed
     * @param time notify key time
     */
    void dispatchKeyEvent(UInt16 keyCode, bool goingDown, bool time=true);

public:
    virtual bool init(OSDictionary *dictionary) APPLE_KEXT_OVERRIDE;
    virtual IOService *probe(IOService *provider, SInt32 *score) APPLE_KEXT_OVERRIDE;

    virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
    virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;

    void dispatchMessage(int message, void* data);

    virtual IOReturn setPowerState(unsigned long powerStateOrdinal, IOService * whatDevice) APPLE_KEXT_OVERRIDE;

    friend class YogaSMCUserClient;
};
#endif /* YogaBaseService_hpp */
