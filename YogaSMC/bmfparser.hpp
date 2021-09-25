//  SPDX-License-Identifier: GPL-2.0-only
//
//  bmfparser.hpp
//  YogaSMC
//
//  Created by Zhen on 2020/5/24.
//  Copyright © 2020 Zhen. All rights reserved.
//

//#include "common.h"

#ifndef bmfparser_hpp
#define bmfparser_hpp

#include <IOKit/acpi/IOACPIPlatformDevice.h>

#define BMF_DATA_WMI_BUFFER "05901221-d566-11d1-b2f0-00a0c9062910"

enum mof_offset_type {
  MOF_OFFSET_UNKNOWN,
  MOF_OFFSET_BOOLEAN = 0x01,
  MOF_OFFSET_OBJECT = 0x02,
  MOF_OFFSET_STRING = 0x03,
  MOF_OFFSET_SINT32 = 0x11,
};

enum mof_data_type {
  MOF_UNKNOWN,
  MOF_SINT16 = 0x02, // Unused
  MOF_SINT32 = 0x03,
  MOF_STRING = 0x08,
  MOF_BOOLEAN = 0x0B,
  MOF_OBJECT = 0x0D,
  MOF_SINT8 = 0x10, // Unused
  MOF_UINT8 = 0x11, // Unused
  MOF_UINT16 = 0x12, // Unused
  MOF_UINT32 = 0x13, // Unused
  MOF_SINT64 = 0x14, // Unused
  MOF_UINT64 = 0x15, // Unused
  MOF_DATETIME = 0x65, // Unused
};

class MOF {
    
public:
    MOF(char *data, uint32_t size, OSDictionary *mData, const char* name) : buf(data), size(size), mData(mData), wmi_name(name) {};
    MOF();
    OSObject* parse_bmf();
    bool parsed;
private:
    const char* wmi_name;
    OSString *parse_string(char *buf, uint32_t size);
    uint16_t parse_valuemap(uint16_t *buf, bool map, uint32_t i);
    uint32_t parse_valuemap(int32_t *buf, bool map, uint32_t i);

    OSDictionary* parse_class(uint32_t *buf);
    OSDictionary* parse_method(uint32_t *buf, uint32_t verify = 0);

    int indent;

    char * buf;
    uint32_t size;
    OSArray* valuemap;
    OSDictionary *vmap;
    OSDictionary *mData;
};

#endif /* bmfparser_hpp */
