//
//  YogaHIDD.hpp
//  YogaSMC
//
//  Created by Zhen on 11/4/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef YogaHIDD_hpp
#define YogaHIDD_hpp

#include "YogaVPC.hpp"

// from linux/drivers/platform/x86/intel-hid.c

#define HID_EVENT_FILTER_UUID    "eeec56b3-4442-408f-a792-4edd4d758054"
#define HIDD_DSM_REVISION        1

enum intel_hid_dsm_fn_codes {
    INTEL_HID_DSM_FN_INVALID,
    INTEL_HID_DSM_BTNL_FN,
    INTEL_HID_DSM_HDMM_FN,
    INTEL_HID_DSM_HDSM_FN,
    INTEL_HID_DSM_HDEM_FN,
    INTEL_HID_DSM_BTNS_FN,
    INTEL_HID_DSM_BTNE_FN,
    INTEL_HID_DSM_HEBC_V1_FN,
    INTEL_HID_DSM_VGBS_FN,
    INTEL_HID_DSM_HEBC_V2_FN,
    INTEL_HID_DSM_FN_MAX
};

static const char *intel_hid_dsm_fn_to_method[INTEL_HID_DSM_FN_MAX] = {
    NULL,
    "BTNL",
    "HDMM",
    "HDSM",
    "HDEM",
    "BTNS",
    "BTNE",
    "HEBC",
    "VGBS",
    "HEEC"
};

class YogaHIDD : public YogaVPC
{
    typedef YogaVPC super;
    OSDeclareDefaultStructors(YogaHIDD)

    bool initVPC() APPLE_KEXT_OVERRIDE;
    bool exitVPC() APPLE_KEXT_OVERRIDE;

    bool initDSM();

    /**
     * Evaluate _DSM for specific GUID and function index.
     * @param index Function index
     * @param result The return is a buffer containing one bit for each function index if Function Index is zero, otherwise could be any data object (See 9.1.1 _DSM (Device Specific Method) in ACPI Specification, Version 6.3)
     * @param arg argument array
     * @param uuid Human-readable GUID string (big-endian)
     * @param revision _DSM revision
     *
     * @return *kIOReturnSuccess* upon a successfull *_DSM* parse, otherwise failed when executing *evaluateObject*.
     */
    IOReturn evaluateDSM(UInt32 index, OSObject **result, OSArray *arg=nullptr, const char *uuid=HID_EVENT_FILTER_UUID, UInt32 revision=HIDD_DSM_REVISION);

    /**
     * Evaluate _DSM for specific GUID and function index.
     * @param index Function index
     * @param value pointer to value
     * @param arg argument
     *
     * @return *kIOReturnSuccess* upon a successfull *_DSM* parse, otherwise failed when executing *evaluateObject*.
     */
    IOReturn evaluateHIDD(intel_hid_dsm_fn_codes index, UInt64 *value, SInt64 arg=-1);

    /**
     *  Fn mask
     */
    UInt64 fn_mask {0};

    /**
     *  5 button array or v2 power button capability, will be update on init
     */
    UInt64 arrayCap {0};

    /**
     *  Initialize 5 button array or v2 power button
     */
    void initButtonArray();

public:
    IOReturn message(UInt32 type, IOService *provider, void *argument) APPLE_KEXT_OVERRIDE;
    IOReturn setPowerState(unsigned long powerStateOrdinal, IOService * whatDevice) APPLE_KEXT_OVERRIDE;
};

#endif /* YogaHIDD_hpp */
