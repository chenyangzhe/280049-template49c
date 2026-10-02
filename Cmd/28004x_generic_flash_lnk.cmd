
MEMORY
{
PAGE 0 :
   /* BEGIN is used for the "boot to Flash" bootloader mode   */

   BEGIN           	: origin = 0x080000, length = 0x000002
   RAMM0           	: origin = 0x0000F6, length = 0x00030A

   //RAMALL          	: origin = 0x008000, length = 0x00C000
  RAMALL          	: origin = 0x00C000, length = 0x008000

	RAMLS0123           : origin = 0x008000, length = 0x002000

	RAMLS4567           : origin = 0x00A000, length = 0x002000
   /*
   RAMLS0           : origin = 0x008000, length = 0x000800
   RAMLS1           : origin = 0x008800, length = 0x000800
   RAMLS2           : origin = 0x009000, length = 0x000800
   RAMLS3           : origin = 0x009800, length = 0x000800
   RAMLS4           : origin = 0x00A000, length = 0x000800
   RAMLS5           : origin = 0x00A800, length = 0x000800
   RAMLS6           : origin = 0x00B000, length = 0x000800
   RAMLS7           : origin = 0x00B800, length = 0x000800
*/
   RESET           	: origin = 0x3FFFC0, length = 0x000002


   FLASH_ALL  : origin = 0x080002, length = 0x01FFFE

//   FLASH_BANK1_SEC15_RSVD : origin = 0x09FFF0, length = 0x000010  /* Reserve and do not use for code as per the errata advisory "Memory: Prefetching Beyond Valid Memory" */

PAGE 1 :

   RAMGS3      : origin = 0x012000, length = 0x001FF8
   RAMGS3_RSVD : origin = 0x013FF8, length = 0x000008     /* Reserve and do not use for code as per the errata advisory "Memory: Prefetching Beyond Valid Memory" */

   BOOT_RSVD       : origin = 0x000002, length = 0x0000F1     /* Part of M0, BOOT rom will use this for stack */
   RAMM1           : origin = 0x000400, length = 0x0003F8     /* on-chip RAM block M1 */

   CLA1_MSGRAMLOW   : origin = 0x001480, length = 0x000080
   CLA1_MSGRAMHIGH  : origin = 0x001500, length = 0x000080

//   RAMGS3_RSVD : origin = 0x013FF8, length = 0x000008     /* Reserve and do not use for code as per the errata advisory "Memory: Prefetching Beyond Valid Memory" */
}


SECTIONS
{
   codestart        : > BEGIN,     PAGE = 0, ALIGN(4)
   .text            : > FLASH_ALL,   PAGE = 0, ALIGN(4)
   .cinit           : > FLASH_ALL,     PAGE = 0, ALIGN(4)
   .switch          : > FLASH_ALL,     PAGE = 0, ALIGN(4)
   .reset           : > RESET,     PAGE = 0, TYPE = DSECT /* not used, */

   .stack           : > RAMALL,     PAGE = 0

#if defined(__TI_EABI__)
   .init_array      : > FLASH_ALL,       PAGE = 0,       ALIGN(4)
   .bss             : > RAMALL,       PAGE = 0
   .bss:output      : > RAMALL,       PAGE = 0
   .bss:cio         : > RAMALL,       PAGE = 0
   .data            : > RAMALL,       PAGE = 0
   .sysmem          : > RAMALL,       PAGE = 0
   /* Initalized sections go in Flash */
   .const           : > FLASH_ALL,       PAGE = 0,       ALIGN(4)
#else
   .pinit           : > FLASH_ALL,       PAGE = 0,       ALIGN(4)
   .ebss            : > RAMALL,       PAGE = 0
   .esysmem         : > RAMALL,       PAGE = 0
   .cio             : > RAMALL,       PAGE = 0
   .econst          : > FLASH_ALL,    PAGE = 0, ALIGN(4)
#endif

   ramgs0           : > RAMALL,    PAGE = 0
   ramgs1           : > RAMALL,    PAGE = 0
   ramgs3           : > RAMGS3,    PAGE = 1

// CLA task code
Cla1Prog        :
                  {
                     *(Cla1Prog*)
                  } LOAD = FLASH_ALL,
                    RUN = RAMLS0123,
                    LOAD_START(Cla1ProgLoadStart),
                    RUN_START(Cla1ProgRunStart),
                    LOAD_SIZE(Cla1ProgLoadSize),
                    PAGE = 0, ALIGN(4)

   dclfuncs        : > FLASH_ALL,   PAGE = 0, ALIGN(4)
   dcl32funcs      : > FLASH_ALL,   PAGE = 0, ALIGN(4)

#if defined(__TI_EABI__) 
   .TI.ramfunc      : LOAD = FLASH_ALL,
                      RUN = RAMALL,
                      LOAD_START(RamfuncsLoadStart),
                      LOAD_SIZE(RamfuncsLoadSize),
                      LOAD_END(RamfuncsLoadEnd),
                      RUN_START(RamfuncsRunStart),
                      RUN_SIZE(RamfuncsRunSize),
                      RUN_END(RamfuncsRunEnd),
                      PAGE = 0, ALIGN(4)
#else					  
   .TI.ramfunc      : LOAD = FLASH_ALL,
                      RUN = RAMALL,
                      LOAD_START(_RamfuncsLoadStart),
                      LOAD_SIZE(_RamfuncsLoadSize),
                      LOAD_END(_RamfuncsLoadEnd),
                      RUN_START(_RamfuncsRunStart),
                      RUN_SIZE(_RamfuncsRunSize),
                      RUN_END(_RamfuncsRunEnd),
                      PAGE = 0, ALIGN(4)
#endif

//CLA


    Cla1ToCpuMsgRAM  : > CLA1_MSGRAMLOW,   PAGE = 1
    CpuToCla1MsgRAM  : > CLA1_MSGRAMHIGH,  PAGE = 1

   .scratchpad      : > RAMLS4567,           PAGE = 0
   .bss_cla         : > RAMLS4567,           PAGE = 0

//   Cla1DataRam      : > RAMLS0123,           PAGE = 0
   cla_shared       : > RAMLS4567,           PAGE = 0
//   CLADataLS1       : > RAMLS0123,           PAGE = 0


   .const_cla      : LOAD = FLASH_ALL,
                      RUN = RAMLS4567,
                      RUN_START(Cla1ConstRunStart),
                      LOAD_START(Cla1ConstLoadStart),
                      LOAD_SIZE(Cla1ConstLoadSize),
                      PAGE = 0, ALIGN(4)





}

/*
//===========================================================================
// End of file.
//===========================================================================
*/
