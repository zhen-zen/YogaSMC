# YogaSMC

This driver consists of YogaSMC(WIP), YogaWMI anf YogaVPC.

Each component can be derived for different targets. Support for Yoga series (IdeaPad) is functional. ThinkPad support is read-only, awaiting refactoring and testing.

## YogaSMC (WIP)
Allow syncing SMC keys like sensors reading and battery conservation mode (?).

Pending integration with VirtualSMC and/or VoodooI2CHID.

## YogaWMI
Support for parsing WMI devices (except Thunderbolt).

Based on [the-darkvoid/macOS-IOElectrify](https://github.com/the-darkvoid/macOS-IOElectrify/) ([Dolnor/IOWMIFamily](https://github.com/Dolnor/IOWMIFamily/)) and [bmfparser](https://github.com/zhen-zen/bmfparser) ([pali/bmfdec](https://github.com/pali/bmfdec))

### IdeaWMI
Support Yoga Mode detection and disable keyboard/touchpad.
Fn+esc (obsolete LENOVO_PAPER_LOOKING_EVENT) currently assigned to Fn mode switch.

### ThinkWMI (WIP)
Based on [lenovo/thinklmi](https://github.com/lenovo/thinklmi) ([iksaif/thinkpad-wmi](https://github.com/iksaif/thinkpad-wmi))

## YogaVPC
Intercepting events on vendor-specific devices (VPC) and sync states.
Support set properties using [ioio](https://github.com/RehabMan/OS-X-ioio)

### IdeaVPC
Based on [linux/drivers/platform/x86/ideapad-laptop.c](https://github.com/torvalds/linux/blob/master/drivers/platform/x86/ideapad-laptop.c)
Currently available functions:
Battery conservation mode
Fn lock mode
Reading fn key code (read-only, WIP)

### ThinkVPC (WIP)
Based on [linux/drivers/platform/x86/thinkpad_acpi.c](https://github.com/torvalds/linux/blob/master/drivers/platform/x86/thinkpad_acpi.c)
Currently available functions:
Battery conservation mode (read-only, WIP)
