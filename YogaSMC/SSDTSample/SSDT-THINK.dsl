/*
 * Sample SSDT for ThinkSMC
 */
DefinitionBlock ("", "SSDT", 2, "hack", "Think", 0x00000000)
{
    External (_SB_.PCI0.LPCB.EC.HKEY, DeviceObj)

    /*
     * Optional: Route to customized LED pattern or origin _SI._SST if differ from built in pattern.
     */
    External (_SI_._SST, MethodObj)    // 1 Arguments

    Scope (\_SB.PCI0.LPCB.EC.HKEY)
    {
        // Used as a proxy-method to interface with \_SI._SST in YogaSMC
        Method (CSSI, 1, NotSerialized)
        {
            \_SI._SST (Arg0)
        }
    }

    /*
     * Optional: Sensor access
     * 
     * Double check name of FieldUnit for collision
     * Registers return 0x00 for non-implemented, 
     * and return 0x80 when not available.
     */
    External (_SB.PCI0.LPCB.EC, DeviceObj)    // EC path

    Scope (_SB.PCI0.LPCB.EC)
    {
        OperationRegion (ESEN, EmbeddedControl, Zero, 0x0100)
        Field (ESEN, ByteAcc, Lock, Preserve)
        {
            // TP_EC_THERMAL_TMP0
            Offset (0x78), 
            EST0,   8, // CPU
            EST1,   8, 
            EST2,   8, 
            EST3,   8, // GPU ?
            EST4,   8, // Battery ?
            EST5,   8, // Battery ?
            EST6,   8, // Battery ?
            EST7,   8, // Battery ?

            // TP_EC_THERMAL_TMP8
            Offset (0xC0), 
            EST8,   8, 
            EST9,   8, 
            ESTA,   8, 
            ESTB,   8, 
            ESTC,   8, 
            ESTD,   8, 
            ESTE,   8, 
            ESTF,   8
        }
    }
}

