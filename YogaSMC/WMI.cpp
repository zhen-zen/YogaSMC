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

extern "C" int ds_dec(void* pin,int lin, void* pout, int lout, int flg);

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

bool WMI::initialize()
{
    if (mDevice) {
        name = mDevice->getName();
        mData = OSDictionary::withCapacity(1);
        mEvent = OSDictionary::withCapacity(1);
        if (extractData())
            return true;

        AlwaysLog("WMI method %s not found", kWMIMethod);
    }
    return false;
}

WMI::~WMI()
{
    OSSafeReleaseNULL(mData);
    OSSafeReleaseNULL(mEvent);
}

// Parse the _WDG method output for WMI data blocks
bool WMI::extractData()
{
    OSObject *wdg;
    OSData *data;

    if (mDevice->evaluateObject(kWMIMethod, &wdg) != kIOReturnSuccess) {
        AlwaysLog("Failed to evaluate ACPI object %s", kWMIMethod);
        return false;
    }

    data = OSDynamicCast(OSData, wdg);

    if (!data) {
        AlwaysLog("%s did not return a data blob", kWMIMethod);
        wdg->release();
        return false;
    }

    int count = data->getLength() / WMI_DATA_SIZE;

    for (int i = 0; i < count; i++)
        parseWDGEntry((struct WMI_DATA*)data->getBytesNoCopy(i * WMI_DATA_SIZE, WMI_DATA_SIZE));

    mDevice->setProperty("WDG", mData);
    data->release();
    return true;
}

// Parse WDG datablock into an OSArray
void WMI::parseWDGEntry(struct WMI_DATA* block)
{
    char guid_string[37];
    char notify_id_string[3];
    char object_id_string[3];
    OSDictionary *dict = OSDictionary::withCapacity(7);
    OSObject *value;

    uuid_t hostUUID;

    le_uuid_dec(&block->guid, &hostUUID);
    uuid_unparse_lower(hostUUID, guid_string);

    setPropertyString(dict, kWMIGuid, guid_string);

    if (block->flags & ACPI_WMI_EVENT) {
        setPropertyNumber(dict, kWMINotifyId, block->notify_id, 8);
        snprintf(notify_id_string, 3, "%2x", block->notify_id);
        mEvent->setObject(notify_id_string, dict);
    } else {
        snprintf(object_id_string, 3, "%c%c", block->object_id[0], block->object_id[1]);
        setPropertyString(dict, kWMIObjectId, object_id_string);

        // validate BMF
        if (block->flags == 0 && block->instance_count == 1) {
            DebugLog("Possible BMF object %s %s", guid_string, object_id_string);
            if (!foundBMF) {
                char bmfName[5];
                snprintf(bmfName, 5, ACPIBufferName, object_id_string);

                DebugLog("Validating buffer %s", bmfName);
                if (mDevice->validateObject(bmfName) == kIOReturnSuccess)
                    extractBMF(bmfName);
            }
        }
    }

    setPropertyNumber(dict, kWMIInstanceCount, block->instance_count, 8);
    setPropertyNumber(dict, kWMIFlags, block->flags, 8);
#ifdef DEBUG
    value = parseWMIFlags(block->flags);
    dict->setObject(kWMIFlagsText, value);
    value->release();
    setPropertyBytes(dict, "raw", block, WMI_DATA_SIZE);
#endif
    mData->setObject(guid_string, dict);
    dict->release();
}

OSDictionary* WMI::getMethod(const char * guid, UInt8 flg)
{
    if (mData && mData->getCount() > 0)
    {
        OSDictionary *entry = OSDynamicCast(OSDictionary, mData->getObject(guid));
        if (!entry)
            return nullptr;

        DebugLog("GUID %s matched, verifying flag %d", guid, flg);

        OSNumber *flags = OSDynamicCast(OSNumber, entry->getObject(kWMIFlags));
        if (!flg | (flags->unsigned32BitValue() & flg))
            return entry;
    }
    
    return nullptr;
}

bool WMI::hasMethod(const char * guid, UInt8 flg)
{
    OSDictionary *method = getMethod(guid, flg);
    
    if (!method) {
        return false;
    }

    if (flg == ACPI_WMI_EVENT) {
        DebugLog("Found event with guid %s", guid);
        return true;
    }

    OSString *methodId = OSDynamicCast(OSString, method->getObject(kWMIObjectId));

    if (methodId) {
        DebugLog("Found method %s with guid %s", methodId->getCStringNoCopy(), guid);
        return true;
    }

    return false;
}

bool WMI::executeMethod(const char * guid, OSObject ** result, OSObject * params[], IOItemCount paramCount)
{
    char methodName[5];

    OSDictionary *method = getMethod(guid);
    if (!method) return false;

    OSNumber *flags = OSDynamicCast(OSNumber, method->getObject(kWMIFlags));
    if (flags == nullptr) return false;

    OSString *objectId = OSDynamicCast(OSString, method->getObject(kWMIObjectId));
    OSNumber *notifyId = OSDynamicCast(OSNumber, method->getObject(kWMINotifyId));

    if (flags->unsigned8BitValue() & ACPI_WMI_METHOD) {
        if (!objectId)
            return false;
        snprintf(methodName, 5, ACPIMethodName, objectId->getCStringNoCopy());
    } else if (flags->unsigned8BitValue() & ACPI_WMI_STRING) {
        if (!objectId)
            return false;
        snprintf(methodName, 5, ACPIBufferName, objectId->getCStringNoCopy());
    } else if (flags->unsigned8BitValue() & ACPI_WMI_EVENT) {
        if (notifyId == nullptr)
            return false;
        snprintf(methodName, 5, ACPIEventName, notifyId->unsigned8BitValue());
        if (mDevice->validateObject(methodName) == kIOReturnNotFound)
            return true;
    } else {
        DebugLog("Type 0x%x not implemented", flags->unsigned8BitValue());
        return false;
    }

    DebugLog("Calling method %s", methodName);
    if (mDevice->evaluateObject(methodName, result, params, paramCount) != kIOReturnSuccess)
        return false;

    return true;
}

bool WMI::executeInteger(const char * guid, UInt32 * result, OSObject * params[], IOItemCount paramCount)
{
    bool ret = false;
    OSObject *obj = nullptr;
    if (executeMethod(guid, &obj, params, paramCount)) {
        OSNumber *num = OSDynamicCast(OSNumber, obj);
        if (num != nullptr) {
            ret = true;
            *result = num->unsigned32BitValue();
        }
    }
    if (obj) obj->release();
    return ret;
}

bool WMI::enableEvent(const char * guid, bool enable)
{
    OSObject *params[] = {
        OSNumber::withNumber(enable, 8)
    };

    bool ret = executeMethod(guid, nullptr, params, 1);

    params[0]->release();
    return ret;
}

bool WMI::getEventData(UInt32 event, OSObject **result)
{
    OSObject *params[] = {
        OSNumber::withNumber(event, 32)
    };

    IOReturn ret = mDevice->evaluateObject(ACPINotifyName, result, params, 1);
    params[0]->release();

    return (ret == kIOReturnSuccess);
}

bool WMI::extractBMF(const char *guid_string)
{
    OSObject *bmf;
    OSData *data;

    char methodName[5];

    OSDictionary *method = getMethod(guid_string);
    OSString *objectId = OSDynamicCast(OSString, method->getObject(kWMIObjectId));

    snprintf(methodName, 5, ACPIBufferName, objectId->getCStringNoCopy());
    if (mDevice->evaluateObject(methodName, &bmf) != kIOReturnSuccess) {
        AlwaysLog("Failed to evaluate ACPI object %s for BMF data", methodName);
        return false;
    }

    if (!(data = OSDynamicCast(OSData, bmf))) {
        AlwaysLog("%s did not return a data blob", methodName);
        return false;
    }

    uint32_t len = data->getLength();
    if (len <= 16) {
        AlwaysLog("%s too short", methodName);
        return false;
    }

    uint32_t *pin = (uint32_t *)(data->getBytesNoCopy());
    if (pin[0] != 0x424D4F46 || pin[1] != 0x01 || pin[2] != len-16) {
        AlwaysLog("%s format invalid", methodName);
        return false;
    }

    foundBMF = true;
#ifdef DEBUG
    mDevice->setProperty("BMF size", data->getLength(), sizeof(unsigned int)*8);
#endif
    uint32_t size = pin[3];
    char *pout = new char[size];
    if (ds_dec((char *)pin+16, len-16, pout, size, 0) != size) {
        AlwaysLog("%s Decompress failed", methodName);
        return false;
    }
    pin = nullptr;
#ifdef DEBUG
    mDevice->setProperty("MOF size", size, sizeof(uint32_t)*8);
#endif
    MOF mof(pout, size, mData, name);
    mDevice->removeProperty("MOF");

    OSObject *result = mof.parse_bmf(guid_string);
    mDevice->setProperty("MOF", result);
    if (!mof.parsed)
        mDevice->setProperty("BMF data", data);
    OSSafeReleaseNULL(result);
    OSSafeReleaseNULL(data);

    delete[] pout;
    return true;
}
