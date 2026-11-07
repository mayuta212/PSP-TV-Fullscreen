/* PSP firmware 6.61 / 04g native LCDC imports. */
    .set noreorder
#include "pspimport.s"
    IMPORT_START "sceLcdc_driver",0x00090000
    IMPORT_FUNC "sceLcdc_driver",0x451FE1A1,sceLcdc_driver_451FE1A1
    IMPORT_FUNC "sceLcdc_driver",0xE9DBD35F,sceLcdc_driver_E9DBD35F
