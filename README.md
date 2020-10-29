# YogaSMC ![CI](https://github.com/zhen-zen/YogaSMC/workflows/CI/badge.svg) [![Join the chat at https://gitter.im/YogaSMC/community](https://badges.gitter.im/YogaSMC/community.svg)](https://gitter.im/YogaSMC/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

This driver consists of YogaSMC, YogaWMI and YogaVPC.

Each component can be derived for different targets. Currently IdeaPad and ThinkPad series are supported.

Command to driver can be sent with [ioio](https://github.com/RehabMan/OS-X-ioio), e.g. `ioio -s IdeaVPC ConservationMode true`.

The driver will update the status in ioreg, while details are available in system log, e.g. `log stream --predicate 'senderImagePath contains "YogaSMC"'`. 

Companion userspace apps, YogaSMCPane and YogaSMCNC are also available with GUI configuration and notification service.

## YogaSMC
Allow syncing SMC keys like sensors reading and battery conservation mode.

Based on [acidanthera/VirtualSMC](https://github.com/acidanthera/VirtualSMC/)

### Customized sensor reading
The EC field name for corresponding SMC key is read from Info.plist. If there's no `FieldUnit` object at desired offset, you can add an `OperationRegion` like `SSDT-THINK.dsl` in `SSDTSample`.

### Fan control (WIP)

## YogaWMI
Support for parsing WMI devices and properties. On some devices, it could act as YogaVPC with access to extensive device control method.

(For Thunderbolt WMI interface, see [al3xtjames/ThunderboltPkg](https://github.com/al3xtjames/ThunderboltPkg) instead.)

Based on [the-darkvoid/macOS-IOElectrify](https://github.com/the-darkvoid/macOS-IOElectrify/) ([Dolnor/IOWMIFamily](https://github.com/Dolnor/IOWMIFamily/)) and [bmfparser](https://github.com/zhen-zen/bmfparser) ([pali/bmfdec](https://github.com/pali/bmfdec))

### IdeaWMI
Currently available functions:
- Yoga Mode detection and disabling keyboard/touchpad when flipped
- Extra battery information (require same battery patch to related methods)
- Fn+esc (obsolete paper looking function), currently assigned to Fn mode switch.

### ThinkWMI (WIP)
~~Based on [lenovo/thinklmi](https://github.com/lenovo/thinklmi) ([iksaif/thinkpad-wmi](https://github.com/iksaif/thinkpad-wmi))~~

## YogaVPC
Intercepting events on vendor-specific Virtual Power Controller (VPC) devices and sync states, some instructions are on [project boards](https://github.com/zhen-zen/YogaSMC/projects/).

Currently available functions:
- EC reading
- DYTC setting (available for idea/think, might need appropriate OS version for XOSI)
- Automatic backlight and LED control
- Clamshell mode (need additional patch on `_LID` like  `SSDT-RCSM.dsl` in `SSDTSample`)

### EC reading:
When [Rehabman's](https://www.tonymacx86.com/threads/guide-how-to-patch-dsdt-for-working-battery-status.116102/) battery patching method `RE1B` `RECB` present (or  `SSDT-ECRW.dsl` in `SSDTSample`), desired EC fields can be read using following commands:

- One byte at specific offset: `ioio -s YogaVPC ReadECOffset 0xA4` for field at offset `0xA4`
- Bulk reading: `ioio -s YogaVPC ReadECOffset 0x1006` for `0x10` bytes at offset `0x06` (add total bytes to read before offset)
- Dump whole EC area: `ioio -s YogaVPC ReadECOffset 0x10000`
- Known EC field name: `ioio -s YogaVPC ReadECName B1CY` (no larger than 1 byte due to OS constraint)

### IdeaVPC
Based on [linux/drivers/platform/x86/ideapad-laptop.c](https://github.com/torvalds/linux/blob/master/drivers/platform/x86/ideapad-laptop.c), targets `VPC0` using `_HID` `VPC2004`.

Currently available functions:
- Hotkey polling
- Battery conservation mode
- Quick charge mode (need testing)
- Fn lock mode
- …

### ThinkVPC
Based on [linux/drivers/platform/x86/thinkpad_acpi.c](https://github.com/torvalds/linux/blob/master/drivers/platform/x86/thinkpad_acpi.c), targets `HKEY` using `_HID` `LEN0268`.

Currently available functions:
- Hotkey polling
- Battery conservation mode
- Basic Fan control (WIP)
- HW Mute status (read-only)
- Audio / Mic Mute LED
- …

## YogaSMCPane
The preference pane provides a graphical user interface for basic information and settings, such as battery conservation mode and backlight.

## YogaSMCNC
The notification application receives EC events and displays them on OSD. Corresonding actions will also be triggered for function keys. The configuration can be customized at `~/Library/Preferences/org.zhen.YogaSMC.plist` after closing the app.  

For new events, feel free to submit a PR like [#40](https://github.com/zhen-zen/YogaSMC/pull/40).

## Installation
The kext should work out-of-the-box. If you have modified `_QXX` methods before, please remove the patches.

Some features may relay on methods accessing EC. Although it won't affect the core functionality, please consider patching related EC fields larger than 8-bit.

The `YogaSMCAlter.kext` is a variant without SMC keys support and the dependencies of `Lilu` and `VirtualSMC`. It's designed for quick loading / unloading without reboot when debugging. 

## Credits
- [Apple](https://www.apple.com) for macOS
- [Linux](https://www.linux.org) for [ideapad-laptop](https://github.com/torvalds/linux/blob/master/drivers/platform/x86/ideapad-laptop.c) and [thinkpad-acpi](https://github.com/torvalds/linux/blob/master/drivers/platform/x86/thinkpad_acpi.c) kernel module  
- [RehabMan](https://github.com/RehabMan) for [OS-X-Voodoo-PS2-Controller](https://github.com/RehabMan/OS-X-Voodoo-PS2-Controller), [OS-X-ACPI-Debug](https://github.com/RehabMan/OS-X-ACPI-Debug), [OS-X-ioio](https://github.com/RehabMan/OS-X-ioio) and DSDT patches
- [vit9696](https://github.com/vit9696) for [VirtualSMC](https://github.com/acidanthera/VirtualSMC)
- [the-darkvoid](https://github.com/the-darkvoid) for [macOS-IOElectrify](https://github.com/the-darkvoid/macOS-IOElectrify)
- [pali](https://github.com/pali) for [bmfdec](https://github.com/pali/bmfdec)
- [benbender](https://github.com/benbender), [1Revenger1](https://github.com/1Revenger1) and other contributors for testing and feedback 
