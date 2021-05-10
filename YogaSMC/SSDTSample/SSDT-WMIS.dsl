/*
 * Sample SSDT to fix sensor return
 *
 * Certain models forget to return result from ThermalZone:
 *
 * Method (WQBI, 1, NotSerialized)
 * {
 *     \_TZ.WQBI (Arg0)
 * }
 *
 * So we have to patch it for correct reporting.
 * Rename Method (WQBI, 1, N) to XQBI
 * (ThermalZone one usually has Serialized type)
 *
 * Find: 57514249 01 // WQBI
 * Repl: 58514249 01 // XQBI 
 *
 * MethodFlags :=
 * bit 0-2: ArgCount (0-7)
 * bit 3:   SerializeFlag
 *          0 NotSerialized
 *          1 Serialized
 */
DefinitionBlock ("", "SSDT", 2, "hack", "WMIS", 0x00000000)
{
    External (_TZ.WQBI, MethodObj)    // Method in ThermalZone

    Method (_SB.WMIS.WQBI, 1, NotSerialized)
    {
        Return (\_TZ.WQBI (Arg0))
    }
}

