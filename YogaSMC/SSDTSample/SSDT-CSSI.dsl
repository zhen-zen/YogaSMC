/*
 * Sample DSDT to proxy system state information (_SSI)
 */
DefinitionBlock ("", "SSDT", 2, "hack", "CSSI", 0x00000000)
{
    External (_SB_.PCI0.LPCB.EC__.HKEY, DeviceObj)
    External (_SI_._SST, MethodObj)    // 1 Arguments

    Scope (\_SB.PCI0.LPCB.EC.HKEY)
    {
        // Used as a proxy-method to interface with \_SI._SST in YogaSMC
        Method (CSSI, 1, NotSerialized)
        {
            \_SI._SST (Arg0)
        }
    }
}
