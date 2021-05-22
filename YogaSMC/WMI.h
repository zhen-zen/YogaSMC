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
 */

#ifndef WMI_h
#define WMI_h

#include <IOKit/acpi/IOACPIPlatformDevice.h>

#define kWMIGuid "guid"
#define kWMIObjectId "object-id"
#define kWMINotifyId "notify-id"
#define kWMIInstanceCount "instance-count"
#define kWMIFlags "flags"
#define kWMIFlagsText "flags-text"

#define TBT_WMI_GUID  "86ccfd48-205e-4a77-9c48-2021cbede341"

#define ACPIBufferName  "WQ%s" // MOF
#define ACPIDataSetName "WS%s" // Arg0 = index, Arg1 = buffer
#define ACPIMethodName  "WM%s" // ACPI_WMI_METHOD, Arg0 = index, Arg1 = ID, Arg2 = input
#define ACPIEventName   "WE%02X" // ACPI_WMI_EXPENSIVE, Arg0 = 0 to disable and 1 to enable
#define ACPICollectName "WC%s" // Arg0 = 0 to disable and 1 to enable
#define ACPINotifyName  "_WED" // Arg0 = notification code

/*
 * If the GUID data block is marked as expensive, we must enable and
 * explicitily disable data collection.
 */

enum {
    ACPI_WMI_EXPENSIVE = 0x1,
    ACPI_WMI_METHOD    = 0x2,
    ACPI_WMI_STRING    = 0x4,
    ACPI_WMI_EVENT     = 0x8
};

class WMI
{
    IOACPIPlatformDevice* mDevice {nullptr};
    OSDictionary* mData = {nullptr};
    OSDictionary* mEvent = {nullptr};
    const char* iname;

public:
    // Constructor
    WMI(IOService *provider) : mDevice(OSDynamicCast(IOACPIPlatformDevice, provider)) {};
    // Destructor
    ~WMI();

    bool initialize();
    void start();
    inline const char *getName() {return mDevice->getName();}

    bool hasMethod(const char * guid, UInt8 flg = ACPI_WMI_METHOD);
    bool enableEvent(const char * guid, bool enable);
    bool executeMethod(const char * guid, OSObject ** result = 0, OSObject * params[] = 0, IOItemCount paramCount = 0, bool mute = false);
    bool executeInteger(const char * guid, UInt32 * result, OSObject * params[] = 0, IOItemCount paramCount = 0, bool mute = false);
    inline IOACPIPlatformDevice* getACPIDevice() { return mDevice; }
    inline OSDictionary* getEvent() { return mEvent; }
    bool getEventData(UInt32 event, OSObject ** result);
    UInt8 getInstanceCount(const char * guid);

    IOReturn evaluateMethod(const char * guid, UInt8 instance, UInt32 methodId, OSObject ** result = 0, OSObject * input = 0, bool mute = false);
    IOReturn evaluateInteger(const char * guid, UInt8 instance, UInt32 methodId, UInt32 * result, OSObject * input = 0, bool mute = false);
    IOReturn queryBlock(const char * guid, UInt8 instance, OSObject ** result, bool mute = false);

private:
    OSDictionary* getMethod(const char * guid, UInt8 flg = 0, bool mute = false);

    bool extractData();
    void parseWDGEntry(struct WMI_DATA * block);

    bool parseBMF();
};


#endif /* WMI_h */
