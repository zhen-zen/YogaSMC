/*
 *  Released under "The GNU General Public License (GPL-2.0)"
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Based on Dolnor's IOWMIFamily (https://github.com/Dolnor/IOWMIFamily/)
 * and the-darkvoid's revision (https://github.com/the-darkvoid/macOS-IOElectrify/)
 */

#include "bmfdec.h"
#include "bmfparser.hpp"
#include "common.h"

#include "WMI.h"
#include <uuid/uuid.h>

#define kWMIMethod "_WDG"

struct __attribute__((packed)) WMI_DATA
{
    uuid_t guid;
    union {
        char object_id[2];
        struct {
            unsigned char notify_id;
            unsigned char reserved;
        };
    };
    UInt8 instance_count;
    UInt8 flags;
};

#define WMI_DATA_SIZE sizeof(WMI_DATA)

// Convert UUID to little endian
void le_uuid_dec(uuid_t *in, uuid_t *out)
{
    *((uint32_t *)out + 0) = OSSwapInt32(*((uint32_t *)in + 0));
    *((uint16_t *)out + 2) = OSSwapInt16(*((uint16_t *)in + 2));
    *((uint16_t *)out + 3) = OSSwapInt16(*((uint16_t *)in + 3));
    *((uint64_t *)out + 1) =            (*((uint64_t *)in + 1));
}

// parseWMIFlags - Parse WMI flags to a string
OSString * parseWMIFlags(UInt8 flags)
{
    char output[80];

    memset(&output, 0, sizeof(output));

    if (flags != 0)
    {
        if (flags & ACPI_WMI_EXPENSIVE)
        {
            strlcat(output, "ACPI_WMI_EXPENSIVE ", 80);
        }
        
        if (flags & ACPI_WMI_METHOD)
        {
            strlcat(output, "ACPI_WMI_METHOD ", 80);
        }
        
        if (flags & ACPI_WMI_STRING)
        {
            strlcat(output, "ACPI_WMI_STRING ", 80);
        }
        
        if (flags & ACPI_WMI_EVENT)
        {
            strlcat(output, "ACPI_WMI_EVENT ", 80);
        }
    }
    else
    {
        strlcat(output, "NONE ", 6);
    }

    return (OSString::withCString(output));
}

WMI::WMI(IOService* provider)
{
    mDevice = OSDynamicCast(IOACPIPlatformDevice, provider);
}

bool WMI::initialize()
{
    if (mDevice) {
        mData = OSDictionary::withCapacity(0);
            
        if (extractData()) {
            return true;
        }

        AlwaysLog("WMI method %s not found on object %s\n", kWMIMethod, mDevice->getName());
    }
    return false;
}

WMI::~WMI()
{
    OSSafeReleaseNULL(mData);
}

// Parse the _WDG method output for WMI data blocks
bool WMI::extractData()
{
    OSObject *wdg;
    OSData *data;

    if (mDevice->evaluateObject(kWMIMethod, &wdg) != kIOReturnSuccess)
    {
        AlwaysLog("ACPI object %s does not export _WDG data\n", mDevice->getName());
        return false;
    }
    
    data = OSDynamicCast(OSData, wdg);
    
    if (!data) {
        AlwaysLog("%s:_WDG did not return a data blob\n", mDevice->getName());
        return false;
    }

    int count = data->getLength() / WMI_DATA_SIZE;
    
    for (int i = 0; i < count; i++) {
        parseWDGEntry(
          (struct WMI_DATA*)data->getBytesNoCopy(i * WMI_DATA_SIZE, WMI_DATA_SIZE));
    }
    
    if (bmf_guid_string)
        extractBMF();

    mDevice->setProperty("WDG", mData);
//#ifdef DEBUG
//    mDevice->setProperty("Events", mEvent);
//#endif
    data->release();
    
    return true;
}

// Parse WDG datablock into an OSArray
void WMI::parseWDGEntry(struct WMI_DATA* block)
{
    char guid_string[37];
    char object_id_string[3];
    OSDictionary *dict = OSDictionary::withCapacity(7);
    OSObject *value;
    
    uuid_t hostUUID;
    
    le_uuid_dec(&block->guid, &hostUUID);
    uuid_unparse_lower(hostUUID, guid_string);

    value = OSString::withCString(guid_string);
    dict->setObject(kWMIGuid, value);
    value->release();

    if (block->flags & ACPI_WMI_EVENT) {
        value = OSNumber::withNumber(block->notify_id, 8);
        dict->setObject(kWMINotifyId, value);
    } else {
        snprintf(object_id_string, 3, "%c%c", block->object_id[0], block->object_id[1]);
        value = OSString::withCString(object_id_string);
        dict->setObject(kWMIObjectId, value);
    }
    value->release();

    value = OSNumber::withNumber(block->instance_count, 8);
    dict->setObject(kWMIInstanceCount, value);
    value->release();
    
    value = OSNumber::withNumber(block->flags, 8);
    dict->setObject(kWMIFlags, value);
    value->release();
    
#ifdef DEBUG
    value = parseWMIFlags(block->flags);
    dict->setObject(kWMIFlagsText, value);
    value->release();
#endif
    if (block->flags == 0 && block->instance_count == 1) {
        AlwaysLog("Possible BMF object %s %s", guid_string, object_id_string);
        // TODO: get BMF guid
        if (bmf_guid_string) {
            AlwaysLog("Previous BMF object %s", bmf_guid_string);
        } else {
            bmf_guid_string = new char[37];
            uuid_unparse_lower(hostUUID, bmf_guid_string);
        }
    }

    mData->setObject(guid_string, dict);
    if (block->flags & ACPI_WMI_EVENT) {
        char notify_id_string[3];
        snprintf(notify_id_string, 3, "%2x", block->notify_id);
        if (!mEvent) {
            mEvent = OSDictionary::withCapacity(1);
        } else {
            mEvent->ensureCapacity(mEvent->getCount()+1);
        }
        mEvent->setObject(notify_id_string, dict);
    }
    dict->release();
}

OSDictionary* WMI::getMethod(const char * guid, UInt8 flg)
{
    if (mData && mData->getCount() > 0)
    {
        OSDictionary* entry = OSDynamicCast(OSDictionary, mData->getObject(guid));
        if (!entry)
            return nullptr;
        else
            DebugLog("GUID %s matched, verifying flag %d", guid, flg);

        OSNumber * flags = OSDynamicCast(OSNumber, entry->getObject(kWMIFlags));
        if (!flg | (flags->unsigned32BitValue() & flg))
            return entry;
    }
    
    return nullptr;
}

bool WMI::hasMethod(const char * guid, UInt8 flg)
{
    OSDictionary* method = getMethod(guid, flg);
    
    if (method) {
        if (flg == ACPI_WMI_EVENT) {
            DebugLog("found event with guid %s\n", guid);
            return true;
        }
        OSString * methodId = OSDynamicCast(OSString, method->getObject(kWMIObjectId));

        if (methodId) {
            DebugLog("found method %s with guid %s\n", methodId->getCStringNoCopy(), guid);
            return true;
        }
    }

    return false;
}

bool WMI::executeMethod(const char * guid, OSObject ** result, OSObject * params[], IOItemCount paramCount)
{
    char methodName[5];
    OSDictionary* method = getMethod(guid);

    if (method)
    {
        OSString * methodId = OSDynamicCast(OSString, method->getObject(kWMIObjectId));

        if (methodId) {
            OSNumber * flags = OSDynamicCast(OSNumber, method->getObject(kWMIFlags));
            if (flags->unsigned8BitValue() & ACPI_WMI_METHOD) {
                snprintf(methodName, 5, ACPIMethodName, methodId->getCStringNoCopy());
            } else if (flags->unsigned8BitValue() & ACPI_WMI_STRING) {
                snprintf(methodName, 5, ACPIBufferName, methodId->getCStringNoCopy());
            } else {
                DebugLog("Type 0x%x not available for method %s\n", flags->unsigned8BitValue(), methodId->getCStringNoCopy());
                return false;
            }

            DebugLog("Calling method %s\n", methodName);
            if (mDevice->evaluateObject(methodName, result, params, paramCount) == kIOReturnSuccess)
                return true;
        }
    }

    return false;
}

bool WMI::executeinteger(const char * guid, UInt32 * result, OSObject * params[], IOItemCount paramCount)
{
    char methodName[5];
    OSDictionary* method = getMethod(guid, ACPI_WMI_METHOD);

    if (method)
    {
        OSString * methodId = OSDynamicCast(OSString, method->getObject(kWMIObjectId));

        if (methodId) {
            snprintf(methodName, 5, ACPIMethodName, methodId->getCStringNoCopy());

            DebugLog("Calling method %s\n", methodName);
            if (mDevice->evaluateInteger(methodName, result, params, paramCount) == kIOReturnSuccess)
                return true;
        }
    }

    return false;
}

bool WMI::extractBMF()
{
    OSObject *bmf;
    OSData *data;

    char methodName[5]; // always 4 chars per ACPI spec

    OSDictionary* method = getMethod(bmf_guid_string, 0);
    if (!method)
        return false;

    OSString * methodId = OSDynamicCast(OSString, method->getObject(kWMIObjectId));

    snprintf(methodName, 5, ACPIBufferName, methodId->getCStringNoCopy());

    DebugLog("Evaluating buffer %s\n", methodName);
    if (mDevice->evaluateObject(methodName, &bmf) != kIOReturnSuccess)
    {
        AlwaysLog("%s: ACPI object %s does not export BMF data\n", mDevice->getName(), methodName);
        return false;
    }
    
    data = OSDynamicCast(OSData, bmf);
    
    if (!data) {
        AlwaysLog("%s: %s did not return a data blob\n", mDevice->getName(), methodName);
        return false;
    }
    
    uint32_t len = data->getLength();
    
    if (len <= 16)
    {
        AlwaysLog("%s: %s too short\n", mDevice->getName(), methodName);
        return false;
    }
    
    uint32_t *pin = (uint32_t *)(data->getBytesNoCopy());

    if (pin[0] != 0x424D4F46 || pin[1] != 0x01 || pin[2] != len-16)
    {
        AlwaysLog("%s: %s format invalid\n", mDevice->getName(), methodName);
        return false;
    }

    mDevice->setProperty("BMF size", data->getLength(), sizeof(unsigned int)*8);

    uint32_t size = pin[3];
    char *pout = new char[size];
    if (ds_dec((char *)pin+16, len-16, pout, size, 0) != size) {
        AlwaysLog("%s: %s Decompress failed\n", mDevice->getName(), methodName);
        return false;
    }
    pin = nullptr;

    mDevice->setProperty("MOF size", size, sizeof(uint32_t)*8);
        
    MOF mof(pout, size, mData);
    mDevice->removeProperty("MOF");
    mDevice->removeProperty("BMF data");
    OSObject *result = mof.parse_bmf(bmf_guid_string);
    mDevice->setProperty("MOF", result);
    if (!mof.parsed)
        mDevice->setProperty("BMF data", data);
//    else
    OSSafeReleaseNULL(data);
    
    OSSafeReleaseNULL(result);
    delete[] pout;
    
    return true;
}
