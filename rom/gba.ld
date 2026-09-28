/* Squishy Isle: GBA cartridge linker script.
 * Code and constants live in ROM. .iwram (fast ARM code) and .data are
 * copied to IWRAM at boot, .ewram to EWRAM; .bss and .sbss are cleared. */
OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

MEMORY {
    rom   : ORIGIN = 0x08000000, LENGTH = 32M
    iwram : ORIGIN = 0x03000000, LENGTH = 32K
    ewram : ORIGIN = 0x02000000, LENGTH = 256K
}

__iwram_top = ORIGIN(iwram) + LENGTH(iwram);
__sp_irq = __iwram_top - 0x60;       /* 0x03007FA0, BIOS convention */
__sp_usr = __sp_irq - 0xA0;          /* 0x03007F00 */

SECTIONS {
    .crt0 : { KEEP(*(.crt0)) } >rom

    .text : {
        *(.text .text.* .gnu.linkonce.t.*)
        *(.glue_7 .glue_7t .vfp11_veneer .v4_bx)
        . = ALIGN(4);
    } >rom

    .rodata : {
        KEEP(*(.rodata.keep))
        *(.rodata .rodata.* .gnu.linkonce.r.*)
        . = ALIGN(4);
    } >rom

    .ARM.exidx : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) } >rom

    .iwram : ALIGN(4) {
        __iwram_start = .;
        *(.iwram .iwram.*)
        . = ALIGN(4);
        __iwram_end = .;
    } >iwram AT>rom
    __iwram_lma = LOADADDR(.iwram);

    .data : ALIGN(4) {
        __data_start = .;
        *(.data .data.* .gnu.linkonce.d.*)
        . = ALIGN(4);
        __data_end = .;
    } >iwram AT>rom
    __data_lma = LOADADDR(.data);

    .bss (NOLOAD) : ALIGN(4) {
        __bss_start = .;
        *(.bss .bss.* .gnu.linkonce.b.* COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } >iwram

    .ewram : ALIGN(4) {
        __ewram_start = .;
        *(.ewram .ewram.*)
        . = ALIGN(4);
        __ewram_end = .;
    } >ewram AT>rom
    __ewram_lma = LOADADDR(.ewram);

    .sbss (NOLOAD) : ALIGN(4) {
        __sbss_start = .;
        *(.sbss .sbss.*)
        . = ALIGN(4);
        __sbss_end = .;
    } >ewram

    end = .;
    __end__ = .;

    /DISCARD/ : { *(.note.*) *(.comment) }
}
