/* Registry read API, exported by PSP firmware 6.61. */
    .set noreorder
#include "pspimport.s"
    IMPORT_START "sceReg",0x40010000
    IMPORT_FUNC "sceReg",0x92E41280,sceRegOpenRegistry
    IMPORT_FUNC "sceReg",0x1D8A762E,sceRegOpenCategory
    IMPORT_FUNC "sceReg",0x30BE0259,sceRegGetKeyValueByName
    IMPORT_FUNC "sceReg",0x0CAE832B,sceRegCloseCategory
    IMPORT_FUNC "sceReg",0xFA8A5739,sceRegCloseRegistry
