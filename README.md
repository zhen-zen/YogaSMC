# YogaSMC ![CI](https://github.com/zhen-zen/YogaSMC/workflows/CI/badge.svg) [![Join the chat at https://gitter.im/YogaSMC/community](https://badges.gitter.im/YogaSMC/community.svg)](https://gitter.im/YogaSMC/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

This driver consists of YogaSMC (WIP), YogaWMI and YogaVPC.

Each component can be derived for different targets. Support for Yoga series (IdeaPad) is functional. ThinkPad support is read-only, awaiting further testing.

Currently it's possible to interact with the driver using [ioio](https://github.com/RehabMan/OS-X-ioio), e.g. `ioio -s IdeaVPC ConservationMode true`.

The driver will update the result in ioreg, while details can be monitored using `log stream --predicate 'processID=0 && senderImagePath contains "YogaSMC"`. 

## YogaSMC (WIP)
Allow syncing SMC keys like sensors reading and battery conservation mode.

Based on [acidanthera/VirtualSMC](https://github.com/acidanthera/VirtualSMC/)

### Customized sensor reading
The EC field name for corresponding SMC key is read from Info.plist. If there's no `FieldUnit` object at desired offset, you can add an `OperationRegion` like `SSDT-THINK.dsl` in `SSDTSample`.

## YogaWMI
Support for parsing WMI devices and properties.

(For Thunderbolt WMI interface, see [al3xtjames/ThunderboltPkg](https://github.com/al3xtjames/ThunderboltPkg) instead.)

Based on [the-darkvoid/macOS-IOElectrify](https://github.com/the-darkvoid/macOS-IOElectrify/) ([Dolnor/IOWMIFamily](https://github.com/Dolnor/IOWMIFamily/)) and [bmfparser](https://github.com/zhen-zen/bmfparser) ([pali/bmfdec](https://github.com/pali/bmfdec))

### IdeaWMI
Support Yoga Mode detection, extra battery information and disabling keyboard/touchpad when flipped.

Fn+esc (obsolete paper looking function) currently assigned to Fn mode switch.

### ThinkWMI (WIP)
Based on [lenovo/thinklmi](https://github.com/lenovo/thinklmi) ([iksaif/thinkpad-wmi](https://github.com/iksaif/thinkpad-wmi))

## YogaVPC
Intercepting events on vendor-specific Virtual Power Controller (VPC) devices and sync states, some instructions are on [project boards](https://github.com/zhen-zen/YogaSMC/projects/).

Currently available functions:
- EC reading
- DYTC setting (available for idea/think, might need appropriate OS version for XOSI)
- Automatic Keyboard backlight: 
  -  BIT 0: Triggered by sleep/wake
  -  BIT 1: Triggered by yoga mode change
  -  BIT 2: _SI._SST
  -  BIT 3: Mute LED
  -  BIT 4: Mic Mute LED
- Clamshell mode (need additional patch on `_LID` like  `SSDT-RCSM.dsl` in `SSDTSample`)

### EC reading:
When [Rehabman's](https://www.tonymacx86.com/threads/guide-how-to-patch-dsdt-for-working-battery-status.116102/) battery patching method `RE1B` `RECB` present (or  `SSDT-ECRW.dsl` in `SSDTSample`), desired EC fields can be read using following commands:

- One byte at specific offset: `ioio -s YogaVPC ReadECOffset 0xA4` for field at offset `0xA4`
- Bulk reading: `ioio -s YogaVPC ReadECOffset 0x1006` for `0x10` bytes at offset `0x06` (add total bytes to read before offset)
- Dump whole EC area: `ioio -s YogaVPC ReadECOffset 0x10000`
- Known EC field name: `ioio -s YogaVPC ReadECName B1CY`  (no larger than 1 byte due to OS constraint)

### IdeaVPC
Based on [linux/drivers/platform/x86/ideapad-laptop.c](https://github.com/torvalds/linux/blob/master/drivers/platform/x86/ideapad-laptop.c), targets `VPC0` using `_HID` `VPC2004`.

Currently available functions:
- Hotkey polling
- Battery conservation mode
- Quick charge mode (need testing)
- Fn lock mode

### ThinkVPC
Based on [linux/drivers/platform/x86/thinkpad_acpi.c](https://github.com/torvalds/linux/blob/master/drivers/platform/x86/thinkpad_acpi.c), targets `HKEY` using `_HID` `LEN0268`.

Currently available functions:
- Hotkey polling
- Battery conservation mode
- Basic Fan control (WIP)
- HW Mute status (read-only)
- Audio / Mic Mute LED
