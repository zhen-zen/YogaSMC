//  SPDX-License-Identifier: GPL-2.0-only
//
//  IdeaVPC.hpp
//  YogaSMC
//
//  Created by Zhen on 2020/7/11.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef IdeaVPC_hpp
#define IdeaVPC_hpp

#include "../YogaVPC.hpp"

// from linux/drivers/platform/x86/ideapad-laptop.c

#define BM_CONSERVATION_BIT (5)
#define HA_FNLOCK_BIT       (10)

#define CFG_GRAPHICS_BIT    (8)
#define CFG_BT_BIT    (16)
#define CFG_3G_BIT    (17)
#define CFG_WIFI_BIT    (18)
#define CFG_CAMERA_BIT    (19)

enum {
    BMCMD_CONSERVATION_ON = 3,
    BMCMD_CONSERVATION_OFF = 5,
    HACMD_FNLOCK_ON = 0xe,
    HACMD_FNLOCK_OFF = 0xf,
};

enum {
    VPCCMD_R_VPC1 = 0x10,
    VPCCMD_R_BL_MAX, /* 0x11 */
    VPCCMD_R_BL, /* 0x12 */
    VPCCMD_W_BL, /* 0x13 */
    VPCCMD_R_WIFI, /* 0x14 */
    VPCCMD_W_WIFI, /* 0x15 */
    VPCCMD_R_BT, /* 0x16 */
    VPCCMD_W_BT, /* 0x17 */
    VPCCMD_R_BL_POWER, /* 0x18 */
    VPCCMD_R_NOVO, /* 0x19 */
    VPCCMD_R_VPC2, /* 0x1A */
    VPCCMD_R_TOUCHPAD, /* 0x1B */
    VPCCMD_W_TOUCHPAD, /* 0x1C */
    VPCCMD_R_CAMERA, /* 0x1D */
    VPCCMD_W_CAMERA, /* 0x1E */
    VPCCMD_R_3G, /* 0x1F */
    VPCCMD_W_3G, /* 0x20 */
    VPCCMD_R_ODD, /* 0x21 */
    VPCCMD_W_FAN, /* 0x22 */
    VPCCMD_R_RF, /* 0x23 */
    VPCCMD_W_RF, /* 0x24 */
    VPCCMD_R_FAN = 0x2B,
    VPCCMD_R_SPECIAL_BUTTONS = 0x31,
    VPCCMD_W_BL_POWER = 0x33,
} VPCCMD;

#define IDEAPAD_EC_TIMEOUT (200) /* in ms */

class IdeaVPC : public YogaVPC
{
    typedef YogaVPC super;
    OSDeclareDefaultStructors(IdeaVPC)

private:
    /**
     *  Related ACPI methods
     */
    static constexpr const char *getVPCConfig         = "_CFG";
    static constexpr const char *getConservationMode  = "GBMD";
    static constexpr const char *setConservationMode  = "SBMC";
    static constexpr const char *getFnlockMode        = "HALS";
    static constexpr const char *setFnlockMode        = "SALS";
    static constexpr const char *readVPCStatus        = "VPCR";
    static constexpr const char *writeVPCStatus       = "VPCW";

    /**
     * VPC0 config
     */
    UInt32 config;
    UInt8 cap_graphics;
    bool cap_bt;
    bool cap_3g;
    bool cap_wifi;
    bool cap_camera;

    bool initVPC() APPLE_KEXT_OVERRIDE;
    void setPropertiesGated(OSObject* props) APPLE_KEXT_OVERRIDE;
    void updateAll() APPLE_KEXT_OVERRIDE;
    void updateVPC() APPLE_KEXT_OVERRIDE;

    /**
     *  Battery conservation mode status, will be update on init
     */
    bool conservationMode {false};

    /**
     *  Fn lock mode status, will be update on init
     */
    bool FnlockMode {false};

    /**
     *  Update battery conservation mode status
     *
     *  @param update  only update internal status when false
     *
     *  @return true if success
     */
    bool updateConservation(bool update=true);

    /**
     *  Update Fn lock mode status
     *
     *  @param update  only update internal status when false
     *
     *  @return true if success
     */
    bool updateFnlock(bool update=true);

    /**
     *  Toggle Fn lock mode
     *
     *  @return true if success
     */
    bool toggleFnlock();

    /**
     *  Toggle battery conservation mode
     *
     *  @return true if success
     */
    bool toggleConservation();

    /**
     *  Read EC data
     *
     *  @param cmd see VPCCMD
     *  @param result EC read result
     *  @param retries number of retries when reading, timeout after 5 retires or IDEAPAD_EC_TIMEOUT
     *
     *  @return true if success
     */
    bool read_ec_data(UInt32 cmd, UInt32 *result, UInt8 *retries);

    /**
     *  Write EC data
     *
     *  @param cmd see VPCCMD
     *  @param value EC write value
     *  @param retries number of retries when writing, timeout after 5 retires or IDEAPAD_EC_TIMEOUT
     *
     *  @return true if success
     */
    bool write_ec_data(UInt32 cmd, UInt32 value, UInt8 *retries);

    /**
     *  Write EC data
     *
     *  @param cmd 0 for value, 1 for VMCCMD
     *  @param data EC value or VMCCMD
     *
     *  @return true if success
     */
    bool method_vpcw(UInt32 cmd, UInt32 data);

    /**
     *  Read EC data
     *
     *  @param cmd 1 for status, 0 for value
     *  @param result write status (0) or EC value
     *
     *  @return true if success
     */
    bool method_vpcr(UInt32 cmd, UInt32 *result);

    friend class IdeaWMI;

public:
    static IdeaVPC* withDevice(IOACPIPlatformDevice *device, OSString *pnp);
};

#endif /* IdeaVPC_hpp */
