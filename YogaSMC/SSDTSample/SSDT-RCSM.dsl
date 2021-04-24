/*
 * Sample SSDT to disable lid open
 */
DefinitionBlock ("", "SSDT", 2, "hack", "RCSM", 0x00000000)
{
    External (_SB_.PCI0.LPCB.H_EC, DeviceObj)         // EC path
    External (_SB_.PCI0.LPCB.H_EC.VPC0, DeviceObj)    // VPC path
    External (_SB_.PCI0.LPCB.H_EC.XQ0D, MethodObj)    // Use ACPIDebug to find your lid open EC query and rename that
    External (RMDT.XLID, IntObj)

    Scope (_SB.PCI0.LPCB.H_EC)
    {
        Name (RCSM, Zero)
        Scope (VPC0)
        {
            Method (GCSM, 0, NotSerialized)
            {
                Return (RCSM)
            }

            Method (SCSM, 1, NotSerialized)
            {
                If ((Arg0 == Zero))
                {
                    RCSM = Zero
                }
                Else
                {
                    RCSM = One
                }

                Return (Zero)
            }
        }

        Method (_Q0D, 0, NotSerialized)    // _QXX method for lid open
        {
            If ((RCSM == Zero))
            {
                XQ0D ()                    // Renamed _QXX method for lid open
            }
        }
    }
}

