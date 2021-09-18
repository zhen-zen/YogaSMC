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

#define kWMIMethod      "_WDG"

#define ACPINotifyName  "_WED" // Arg0 = notification code
#define ACPIBufferName  "WQ%s" // MOF
#define ACPIDataSetName "WS%s" // Arg0 = index, Arg1 = buffer
#define ACPIMethodName  "WM%s" // ACPI_WMI_METHOD, Arg0 = index, Arg1 = ID, Arg2 = input
#define ACPIEventName   "WE%02X" // ACPI_WMI_EXPENSIVE, Arg0 = 0 to disable and 1 to enable
#define ACPICollectName "WC%s" // Arg0 = 0 to disable and 1 to enable

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
    *(reinterpret_cast<uint32_t *>(out) + 0) = OSSwapInt32(*(reinterpret_cast<uint32_t *>(in) + 0));
    *(reinterpret_cast<uint16_t *>(out) + 2) = OSSwapInt16(*(reinterpret_cast<uint16_t *>(in) + 2));
    *(reinterpret_cast<uint16_t *>(out) + 3) = OSSwapInt16(*(reinterpret_cast<uint16_t *>(in) + 3));
    *(reinterpret_cast<uint64_t *>(out) + 1) =            (*(reinterpret_cast<uint64_t *>(in) + 1));
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
    if (!mDevice)
        return false;

    iname = mDevice->getName();
    mData = OSDictionary::withCapacity(1);
    mEvent = OSDictionary::withCapacity(1);

    if (!extractData()) {
        AlwaysLog("WMI method %s not found", kWMIMethod);
        return false;
    }

    return true;
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
    }

    setPropertyNumber(dict, kWMIInstanceCount, block->instance_count, 8);
    setPropertyNumber(dict, kWMIFlags, block->flags, 8);
    value = parseWMIFlags(block->flags);
    dict->setObject(kWMIFlagsText, value);
    value->release();
#ifdef DEBUG
    setPropertyBytes(dict, "raw", block, WMI_DATA_SIZE);
#endif
    mData->setObject(guid_string, dict);
    dict->release();
}

OSDictionary* WMI::getMethod(const char * guid, UInt8 flg, bool mute)
{
    if (mData && mData->getCount() > 0)
    {
        OSDictionary *entry = OSDynamicCast(OSDictionary, mData->getObject(guid));
        if (!entry)
            return nullptr;

        if (!mute)
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

    if (!method)
        return false;

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

bool WMI::executeMethod(const char * guid, OSObject ** result, OSObject * params[], IOItemCount paramCount, bool mute)
{
    char methodName[5];

    OSDictionary *method = getMethod(guid, 0, mute);
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
        if (!mute)
            DebugLog("Type 0x%x not implemented, trying buffer type", flags->unsigned8BitValue());
        snprintf(methodName, 5, ACPIBufferName, objectId->getCStringNoCopy());
        if (mDevice->validateObject(methodName) == kIOReturnNotFound)
            return false;
    }

    if (!mute)
        DebugLog("Calling method %s", methodName);
    if (mDevice->evaluateObject(methodName, result, params, paramCount) != kIOReturnSuccess)
        return false;

    return true;
}

bool WMI::executeInteger(const char * guid, UInt32 * result, OSObject * params[], IOItemCount paramCount, bool mute)
{
    bool ret = false;
    OSObject *obj = nullptr;
    if (executeMethod(guid, &obj, params, paramCount, mute)) {
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
        OSNumber::withNumber(enable, 8),
    };

    bool ret = executeMethod(guid, nullptr, params, 1);
    params[0]->release();

    return ret;
}

bool WMI::getEventData(UInt32 event, OSObject **result)
{
    OSObject *params[] = {
        OSNumber::withNumber(event, 32),
    };

    IOReturn ret = mDevice->evaluateObject(ACPINotifyName, result, params, 1);
    params[0]->release();

    return (ret == kIOReturnSuccess);
}

UInt8 WMI::getInstanceCount(const char *guid)
{
    OSDictionary *method = getMethod(guid, 0);

    if (!method)
        return 0;

    OSNumber *instanceCount = OSDynamicCast(OSNumber, method->getObject(kWMIInstanceCount));
    if (instanceCount == nullptr)
        return 0;

    return instanceCount->unsigned8BitValue();
}

void WMI::start()
{
    parseBMF();
    mDevice->setProperty("WDG", mData);
}

bool WMI::parseBMF()
{
    OSObject *bmf = nullptr;
    if (!executeMethod(BMF_DATA_WMI_BUFFER, &bmf)) {
        AlwaysLog("Failed to evaluate BMF data");
        return false;
    }

    OSData *data;
    if (!(data = OSDynamicCast(OSData, bmf))) {
        AlwaysLog("BMF not a data blob");
        return false;
    }

    uint32_t len = data->getLength();
    if (len <= 16) {
        AlwaysLog("BMF too short");
        return false;
    }

    uint32_t *pin = (uint32_t *)(data->getBytesNoCopy());
    if (pin[0] != 0x424D4F46 || pin[1] != 0x01 || pin[2] != len-16) {
        AlwaysLog("BMF format invalid");
        return false;
    }

#ifdef DEBUG
    mDevice->setProperty("BMF size", data->getLength(), sizeof(unsigned int)*8);
#endif
    uint32_t size = pin[3];
    char *pout = new char[size];
    if (ds_dec((char *)pin+16, len-16, pout, size, 0) != size) {
        AlwaysLog("BMF Decompress failed");
        return false;
    }
    pin = nullptr;
#ifdef DEBUG
    mDevice->setProperty("MOF size", size, sizeof(uint32_t)*8);
#endif
    MOF mof(pout, size, mData, iname);
    mDevice->removeProperty("MOF");

    OSObject *result = mof.parse_bmf();
    mDevice->setProperty("MOF", result);
    if (!mof.parsed)
        mDevice->setProperty("BMF data", data);
    OSSafeReleaseNULL(result);
    OSSafeReleaseNULL(data);

    delete[] pout;
    return true;
}

IOReturn WMI::evaluateMethod(const char *guid, UInt8 instance, UInt32 methodId, OSObject ** result, OSObject * input, bool mute) {
    IOReturn ret;

    OSDictionary *method = getMethod(guid, ACPI_WMI_METHOD, mute);
    if (!method) return kIOReturnNotFound;

    OSString *objectId = OSDynamicCast(OSString, method->getObject(kWMIObjectId));
    if (!objectId) return kIOReturnNotFound;

    OSNumber *instanceCount = OSDynamicCast(OSNumber, method->getObject(kWMIInstanceCount));
    if (instanceCount->unsigned8BitValue() <= instance)
        return kIOReturnBadArgument;

    char methodName[5];
    snprintf(methodName, 5, ACPIMethodName, objectId->getCStringNoCopy());

    OSObject* params[3] = {
        OSNumber::withNumber(instance, 8),
        OSNumber::withNumber(methodId, 32),
        input
    };

    ret = mDevice->evaluateObject(methodName, result, params, input ? 3 : 2);
    params[0]->release();
    params[1]->release();

    return ret;
}

IOReturn WMI::evaluateInteger(const char * guid, UInt8 instance, UInt32 methodId, UInt32 * result, OSObject * input, bool mute) {
    IOReturn ret;
    OSObject *obj = nullptr;

    ret = evaluateMethod(guid, instance, methodId, &obj, input, mute);
    if (ret == kIOReturnSuccess) {
        OSNumber *num = OSDynamicCast(OSNumber, obj);
        if (num != nullptr)
            *result = num->unsigned32BitValue();
        else
            ret = kIOReturnBadArgument;
    }

    if (obj) obj->release();
    return ret;
}

IOReturn WMI::queryBlock(const char *guid, UInt8 instance, OSObject ** result, bool mute) {
    IOReturn ret;

    OSDictionary *method = getMethod(guid, 0, mute);
    if (!method) return kIOReturnNotFound;

    OSString *objectId = OSDynamicCast(OSString, method->getObject(kWMIObjectId));
    if (!objectId) return kIOReturnNotFound;

    OSNumber *instanceCount = OSDynamicCast(OSNumber, method->getObject(kWMIInstanceCount));
    if (instanceCount->unsigned8BitValue() <= instance)
        return kIOReturnBadArgument;

    OSNumber *flags = OSDynamicCast(OSNumber, method->getObject(kWMIFlags));
    if (flags->unsigned8BitValue() & (ACPI_WMI_EVENT | ACPI_WMI_METHOD))
        return kIOReturnError;

    char methodName[5];
    snprintf(methodName, 5, ACPIBufferName, objectId->getCStringNoCopy());

    OSObject* params[1] = {
        OSNumber::withNumber(instance, 8),
    };

    ret = mDevice->evaluateObject(methodName, result, params, 1);
    params[0]->release();

    return ret;
}
