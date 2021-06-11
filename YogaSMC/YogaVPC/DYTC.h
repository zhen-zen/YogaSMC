//
//  DYTC.h
//  YogaSMC
//
//  Created by Zhen on 8/26/20.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef DYTC_h
#define DYTC_h

#include <libkern/OSTypes.h>

// based on ibm-acpi-devel

enum {
    DYTC_CMD_QUERY = 0,    /* Get DYTC Version */
    DYTC_CMD_SET   = 1,    /* Set current IC function and mode */
    DYTC_CMD_GET   = 2,    /* Get current IC function and mode */
    DYTC_CMD_QFUNC = 3,    /* Get available IC functions */
    /* 3-7, 0x0100 unknown yet, capability? */
    DYTC_CMD_RESET = 0x1ff,    /* Reset current IC function and mode */
};

// Functions
#define DYTC_FUNCTION_STD     0  /* Function = 0, standard mode */
#define DYTC_FUNCTION_CQL     1  /* Function = 1, lap mode */
#define DYTC_FUNCTION_MMC     11 /* Function = 11, desk mode, priority 3 */

// Functions spotted
#define DYTC_FUNCTION_MYH     3  /* Function = 3, ? mode, priority 5 */
#define DYTC_FUNCTION_STP     4  /* Function = 4, ? mode, priority 1 */
#define DYTC_FUNCTION_DMC     8  /* Function = 8, ? mode */
#define DYTC_FUNCTION_IFC     10 /* Function = 10, ? mode, priority 4 */
#define DYTC_FUNCTION_MSC     12 /* Function = 12, ? mode, priority 2 */
#define DYTC_FUNCTION_PSC     13 /* Function = 13, ? mode */

// Functions deduced
#define DYTC_FUNCTION_TIO     2
#define DYTC_FUNCTION_CQH     5
#define DYTC_FUNCTION_DCC     6
#define DYTC_FUNCTION_SFN     7
#define DYTC_FUNCTION_FHP     9
#define DYTC_FUNCTION_CSC     14

// Availeble mode
#define DYTC_MODE_PERFORM     2  /* High power mode aka performance */
#define DYTC_MODE_QUIET       3  /* low power mode aka quiet */
#define DYTC_MODE_BALANCE   0xF  /* default mode aka balance */

// Error code
#define DYTC_EXCEPTION        0
#define DYTC_SUCCESS          1
#define DYTC_FUNC_INVALID     1 << 1
#define DYTC_CMD_INVALID      2 << 1
#define DYTC_DPTF_UNAVAILABLE 3 << 1
#define DYTC_UNSUPPORTED      4 << 1
#define DYTC_MODE_INVALID     5 << 1

// Deprecated constants

//#define DYTC_QUERY_ENABLE_BIT 8  /* Bit 8 - 0 = disabled, 1 = enabled */
//#define DYTC_QUERY_SUBREV_BIT 16 /* Bits 16 - 27 - sub revisision */
//#define DYTC_QUERY_REV_BIT    28 /* Bits 28 - 31 - revision */

//#define DYTC_GET_FUNCTION_BIT 8  /* Bits 8-11 - function setting */
//#define DYTC_GET_MODE_BIT     12 /* Bits 12-15 - mode setting */
//#define DYTC_GET_LAPMODE_BIT  17 /* Bit 17 - lapmode. Set when on lap */

//#define DYTC_SET_FUNCTION_BIT 12 /* Bits 12-15 - funct setting */
//#define DYTC_SET_MODE_BIT     16 /* Bits 16-19 - mode setting */
//#define DYTC_SET_VALID_BIT    20 /* Bit 20 - 1 = on, 0 = off */

union DYTC_CMD {
    UInt32 raw;
    struct {
        union {
            UInt16 command;
            struct __attribute__((packed)) {
                UInt8 command_lo;
                UInt command_hi: 1;
                UInt unused: 3;
                UInt ICFunc: 4;
            };
        };
        UInt ICMode: 4;
        UInt validF: 1;
    };
};

static const union DYTC_CMD dytc_query_cmd = {.command = DYTC_CMD_QUERY};
static const union DYTC_CMD dytc_set_cmd = {.command = DYTC_CMD_SET};
static const union DYTC_CMD dytc_get_cmd = {.command = DYTC_CMD_GET};
static const union DYTC_CMD dytc_query_func_cmd = {.command = DYTC_CMD_QFUNC};
static const union DYTC_CMD dytc_reset_cmd = {.command = DYTC_CMD_RESET};

#define DYTC_SET_CMD(func, mode, enable) \
    .command = DYTC_CMD_SET, .ICFunc = func, \
    .ICMode = mode, .validF = enable

typedef union {
    UInt32 raw;
    struct {
        UInt8 errorcode;
        union {
            struct __attribute__((packed)) {
                bool enable;
                union {
                    UInt16 version;
                    struct __attribute__((packed)) {
                        UInt8 subrev_lo;
                        UInt subrev_hi: 4;
                        UInt rev: 4;
                    };
                };
            } query;
            struct __attribute__((packed)) {
                UInt funcmode: 4;
                UInt perfmode: 4;
                UInt16 vmode;
            } get;
            UInt16 query_func;
        };
    };
} DYTC_RESULT;

#endif /* DYTC_h */
