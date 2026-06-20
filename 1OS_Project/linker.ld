/* Force Windows linker to drop the 0x400000 default base */
IMAGE_BASE = 0x0;

SECTIONS
{
    /* Tell linker the absolute execution address is 0x7C00 */
    . = 0x7C00;
    
    .text :
    {
        *(.text)
    }
    
    .data : { *(.data) }
    .bss  : { *(.bss)  }
    
    /* Drop standard Windows garbage sections to keep image pure */
    /DISCARD/ : {
        *(.pdata)
        *(.xdata)
        *(.comment)
        *(.note*)
    }
}