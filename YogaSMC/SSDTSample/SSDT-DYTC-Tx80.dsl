/*
 * Sample SSDT for ThinkSMC
 * 
 * Enables DYTC thermal-management on newer Thinkpads
 */
DefinitionBlock ("", "SSDT", 2, "hack", "_DYTC", 0x00000000)
{
    External (WNTF, IntObj)
    
    Scope (\)
    {
        If (_OSI ("Darwin"))
        {
            // Enables DYTC on Thinkpad Tx80, X1C6 and possibly more Thinkpads. Please check \_SB.PCI0.LPCB.EC.HKEY.DYTC()
            WNTF = 0x01
        }
    }
}
