# YogaSMC ![CI](https://github.com/zhen-zen/YogaSMC/workflows/CI/badge.svg) [![Join the chat at https://gitter.im/YogaSMC/community](https://badges.gitter.im/YogaSMC/community.svg)](https://gitter.im/YogaSMC/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

This driver consists of YogaSMC (WIP), YogaWMI and YogaVPC.

Each component can be derived for different targets. Support for Yoga series (IdeaPad) is functional. ThinkPad support is read-only, awaiting further testing.

Currently it's possible to interact with the driver using [ioio](https://github.com/RehabMan/OS-X-ioio), e.g. `ioio -s IdeaVPC ConservationMode true`.

The driver will update the result in ioreg, while details can be monitored using `log stream --predicate 'processID=0 && senderImagePath contains "YogaSMC"`. 

## YogaSMC
Allow syncing SMC keys like sensors reading and battery conservation mode.

### EC reading:
When [Rehabman's](https://www.tonymacx86.com/threads/guide-how-to-patch-dsdt-for-working-battery-status.116102/) battery patching method `RE1B` `RECB` present, desired EC fields can be read using following commands:

- One byte at specific offset: `ioio -s YogaSMC ReadECOffset 0xA4` for field at offset `0xA4`
- Bulk reading: `ioio -s YogaSMC ReadECOffset 0x1006` for `0x10` bytes at offset `0x06` (add total bytes to read before offset)
- Dump whole EC area: `ioio -s YogaSMC ReadECOffset 0x10000` (Dangerous, avoid using it frequently)
- Known EC field name (no larger than 1 byte): `ioio -s YogaSMC ReadECName B1CY`

## YogaWMI
Support for parsing WMI devices and properties.

(For Thunderbolt WMI interface, see [al3xtjames/ThunderboltPkg](https://github.com/al3xtjames/ThunderboltPkg) instead.)

Based on [the-darkvoid/macOS-IOElectrify](https://github.com/the-darkvoid/macOS-IOElectrify/) ([Dolnor/IOWMIFamily](https://github.com/Dolnor/IOWMIFamily/)) and [bmfparser](https://github.com/zhen-zen/bmfparser) ([pali/bmfdec](https://github.com/pali/bmfdec))

### IdeaWMI
Support Yoga Mode detection and disabling keyboard/touchpad when flipped.

Fn+esc (obsolete paper looking function) currently assigned to Fn mode switch.

### ThinkWMI (WIP)
Based on [lenovo/thinklmi](https://github.com/lenovo/thinklmi) ([iksaif/thinkpad-wmi](https://github.com/iksaif/thinkpad-wmi))

## YogaVPC
Intercepting events on vendor-specific Virtual Power Controller (VPC) devices and sync states.

### IdeaVPC
Based on [linux/drivers/platform/x86/ideapad-laptop.c](https://github.com/torvalds/linux/blob/master/drivers/platform/x86/ideapad-laptop.c), targets `VPC0` using `_HID` `VPC2004`.

Currently available functions:
- VPC0 config parsing
- Battery conservation mode
- Fn lock mode
- Keyboard backlight: support automatic on / off on sleep (bit 0) and yoga mode (bit 1)
- Reading fn key code (read-only, WIP)

### ThinkVPC (WIP)
Based on [linux/drivers/platform/x86/thinkpad_acpi.c](https://github.com/torvalds/linux/blob/master/drivers/platform/x86/thinkpad_acpi.c), targets `HKEY` using `_HID` `LEN0268`.

Currently available functions:
- HKEY config parsing
- Battery conservation mode (experimental)
- Fan mode (experimental)
- Mute status (read-only, WIP)
