//  SPDX-License-Identifier: GPL-2.0-only
//
//  IdeaWMI.hpp
//  YogaSMC
//
//  Created by Zhen on 2020/6/23.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef IdeaWMI_hpp
#define IdeaWMI_hpp

#include "YogaWMI.hpp"

#define WBAT_BAT0_BatMaker 0
#define WBAT_BAT0_HwId     1
#define WBAT_BAT0_MfgDate  2
#define WBAT_BAT1_BatMaker 3
#define WBAT_BAT1_HwId     4
#define WBAT_BAT1_MfgDate  5

#define BAT_INFO_WMI_STRING "c3a03776-51ac-49aa-ad0f-f2f7d62c3f3c"
#define GSENSOR_DATA_WMI_METHOD  "09b0ee6e-c3fd-4243-8da1-7911ff80bb8c"
#define GSENSOR_WMI_EVENT "06129d99-6083-4164-81ad-f092f9d773a6"
#define PAPER_LOOKING_WMI_EVENT "56322276-8493-4ce8-a783-98c991274f5e"
enum
{
    kYogaMode_laptop = 1,   // 0-90 degree
    kYogaMode_tablet = 2,   // 0/360 degree
    kYogaMode_stand  = 3,   // 180-360 degree, ∠ , screen face up
    kYogaMode_tent   = 4    // 180-360 degree, ∧ , screen upside down, trigger rotation?
} kYogaMode;

class IdeaWMI : public YogaWMI
{
    typedef YogaWMI super;
    OSDeclareDefaultStructors(IdeaWMI)

private:
    inline virtual bool PMSupport() APPLE_KEXT_OVERRIDE {return true;};

    bool isYMC {false};

    /**
     *  Current Yoga Mode, see kYogaMode
     */
    int YogaMode {kYogaMode_laptop};

    /**
     *  Update Yoga Mode
     */
    void updateYogaMode();

    /**
     *  Parse Battery Info
     *
     *  @param method method name to be executed
     *  @param bat array for status
     *
     *  @return true if success
     */
    bool getBatteryInfo (UInt32 index, OSArray *bat);

    void processWMI() APPLE_KEXT_OVERRIDE;
    void ACPIEvent(UInt32 argument) APPLE_KEXT_OVERRIDE;
    void checkEvent(const char *cname, UInt32 id) APPLE_KEXT_OVERRIDE;

public:
    void stop(IOService *provider) APPLE_KEXT_OVERRIDE;
    static IdeaWMI *withDevice(IOService *provider);

    IOReturn setPowerState(unsigned long powerStateOrdinal, IOService * whatDevice) APPLE_KEXT_OVERRIDE;
};

#endif /* IdeaWMI_hpp */
