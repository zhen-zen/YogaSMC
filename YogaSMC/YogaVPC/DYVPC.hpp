//
//  DYVPC.hpp
//  YogaSMC
//
//  Created by Zhen on 1/7/21.
//  Copyright © 2021 Zhen. All rights reserved.
//

#ifndef DYVPC_hpp
#define DYVPC_hpp

#include "YogaVPC.hpp"

// from linux/drivers/platform/x86/hp-wmi.c

#define INPUT_WMI_EVENT "95f24279-4d7b-4334-9387-accdc67ef61c"
#define BIOS_QUERY_WMI_METHOD "5fb7f034-2c63-45e9-be91-3d44e2c707e4"

enum hp_wmi_radio {
    HPWMI_WIFI    = 0x0,
    HPWMI_BLUETOOTH    = 0x1,
    HPWMI_WWAN    = 0x2,
    HPWMI_GPS    = 0x3,
};

// hpqBEvnt

enum hp_wmi_event_ids {
    HPWMI_DOCK_EVENT        = 0x01,
    HPWMI_PARK_HDD            = 0x02,
    HPWMI_SMART_ADAPTER        = 0x03,
    HPWMI_BEZEL_BUTTON        = 0x04,
    HPWMI_WIRELESS            = 0x05,
    HPWMI_CPU_BATTERY_THROTTLE    = 0x06,
    HPWMI_LOCK_SWITCH        = 0x07,
    HPWMI_LID_SWITCH        = 0x08,
    HPWMI_SCREEN_ROTATION        = 0x09,
    HPWMI_COOLSENSE_SYSTEM_MOBILE    = 0x0A,
    HPWMI_COOLSENSE_SYSTEM_HOT    = 0x0B,
    HPWMI_PROXIMITY_SENSOR        = 0x0C,
    HPWMI_BACKLIT_KB_BRIGHTNESS    = 0x0D,
    HPWMI_PEAKSHIFT_PERIOD        = 0x0F,
    HPWMI_BATTERY_CHARGE_PERIOD    = 0x10,
};

// hpqBIntM

struct bios_args {          // hpqBIOSInt 0 / 4 / 128 / 1024 / 4096
    UInt32 signature;       // 0x1, SNIN, Sign, should be uint8 * 4, 0x55434553 (UCES)
    UInt32 command;         // 0x2, COMD, Command, uint32
    UInt32 commandtype;     // 0x3, CMTP, CommandType, uint32
    UInt32 datasize;        // 0x4, DASI, Size, uint32
    UInt8 data[128];        // 0x5, PVWB, hpqBData, uint8 * Size
};

enum hp_wmi_commandtype {
    HPWMI_DISPLAY_QUERY        = 0x01,
    HPWMI_HDDTEMP_QUERY        = 0x02,
    HPWMI_ALS_QUERY            = 0x03,
    HPWMI_HARDWARE_QUERY        = 0x04,
    HPWMI_WIRELESS_QUERY        = 0x05,
    HPWMI_BATTERY_QUERY        = 0x07,
    HPWMI_BIOS_QUERY        = 0x09,
    HPWMI_FEATURE_QUERY        = 0x0b,
    HPWMI_HOTKEY_QUERY        = 0x0c,
    HPWMI_FEATURE2_QUERY        = 0x0d,
    HPWMI_WIRELESS2_QUERY        = 0x1b,
    HPWMI_POSTCODEERROR_QUERY    = 0x2a,
    HPWMI_THERMAL_POLICY_QUERY    = 0x4c,
};

enum hp_wmi_command {
    HPWMI_READ    = 0x01,
    HPWMI_WRITE    = 0x02,
    HPWMI_ODM    = 0x03,
};

enum hp_wmi_hardware_mask {
    HPWMI_DOCK_MASK        = 0x01,
    HPWMI_TABLET_MASK    = 0x04,
};

struct bios_return {        // hpqBDataOut 0 / 4 / 128 / 1024 / 4096
    UInt32 sigpass;         // 0x1, SNOU, Sign, should be uint8 * 4, 0x4C494146 (LIAF) / 0x53534150 (SSAP)
    UInt32 return_code;     // 0x2, RTCD, rwReturnCode, uint32
    // UInt8 data[];        // 0x3, HVWA, Data, uint8 * 0 / 4 / 128 / 1024 / 4096
};

enum hp_return_value {
    HPWMI_RET_WRONG_SIGNATURE    = 0x02,
    HPWMI_RET_UNKNOWN_COMMAND    = 0x03,
    HPWMI_RET_UNKNOWN_CMDTYPE    = 0x04,
    HPWMI_RET_INVALID_PARAMETERS    = 0x05,
};

enum hp_wireless2_bits {
    HPWMI_POWER_STATE    = 0x01,
    HPWMI_POWER_SOFT    = 0x02,
    HPWMI_POWER_BIOS    = 0x04,
    HPWMI_POWER_HARD    = 0x08,
    HPWMI_POWER_FW_OR_HW    = HPWMI_POWER_BIOS | HPWMI_POWER_HARD,
};

#define IS_HWBLOCKED(x) ((x & HPWMI_POWER_FW_OR_HW) != HPWMI_POWER_FW_OR_HW)
#define IS_SWBLOCKED(x) !(x & HPWMI_POWER_SOFT)

struct bios_rfkill2_device_state {
    UInt8 radio_type;
    UInt8 bus_type;
    UInt16 vendor_id;
    UInt16 product_id;
    UInt16 subsys_vendor_id;
    UInt16 subsys_product_id;
    UInt8 rfkill_id;
    UInt8 power;
    UInt8 unknown[4];
};

/* 7 devices fit into the 128 byte buffer */
#define HPWMI_MAX_RFKILL2_DEVICES    7

struct bios_rfkill2_state {
    UInt8 unknown[7];
    UInt8 count;
    UInt8 pad[8];
    struct bios_rfkill2_device_state device[HPWMI_MAX_RFKILL2_DEVICES];
};

class DYVPC : public YogaVPC
{
    typedef YogaVPC super;
    OSDeclareDefaultStructors(DYVPC)

private:
    bool probeVPC(IOService *provider) APPLE_KEXT_OVERRIDE;
    bool initVPC() APPLE_KEXT_OVERRIDE;
//    void setPropertiesGated(OSObject* props) APPLE_KEXT_OVERRIDE;
//    void updateAll() APPLE_KEXT_OVERRIDE;
    void updateVPC() APPLE_KEXT_OVERRIDE;
    bool exitVPC() APPLE_KEXT_OVERRIDE;

    /**
     *  input capability
     */
    bool inputCap {false};

    /**
     *  BIOS setting capability
     */
    bool BIOSCap {false};

    /**
     *  BIOS setting capability
     *
     *  @param query commandtype
     *  @param buffer result
     *  @param command hp_wmi_command
     *  @param insize input size
     *  @param outsize output size
     *
     *  @return true on success
     */
    bool WMIQuery(UInt32 query, void *buffer, enum hp_wmi_command command = HPWMI_READ, UInt32 insize = sizeof(UInt32), UInt32 outsize = sizeof(UInt32));

public:
    IOReturn message(UInt32 type, IOService *provider, void *argument) APPLE_KEXT_OVERRIDE;
};
#endif /* DYVPC_hpp */
