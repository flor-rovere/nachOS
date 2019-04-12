OUTPUT_FORMAT("ecoff-littlemips")
ENTRY(__start)
SECTIONS
{
    .text 0 : {
        *(.init)
        *(.text)
        *(.fini)
    }
    .data . : {
        /* `coff2noff` cannot handle both `.data` and `.rdata`, so put the
           contents of `.rdata` inside `.data`. */
        *(.rdata)
        *(.data)
        CONSTRUCTORS
    }
    .sbss . : {
        *(.sbss)
        *(.scommon)
    }
    .bss . : {
        *(.bss)
        *(COMMON)
    }
}
