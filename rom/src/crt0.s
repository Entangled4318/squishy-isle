@ Squishy Isle startup code.
@ The 192-byte header area is filled in by tools/gbafix.py after linking.

    .section .crt0, "ax"
    .arm
    .align 2
    .global _start
_start:
    b       reset
    .fill   188, 1, 0               @ logo, title, codes, checksum (0x04-0xBF)

reset:
    mov     r0, #0x12               @ IRQ mode
    msr     cpsr_c, r0
    ldr     sp, =__sp_irq
    mov     r0, #0x1F               @ system mode
    msr     cpsr_c, r0
    ldr     sp, =__sp_usr

    ldr     r0, =__iwram_lma
    ldr     r1, =__iwram_start
    ldr     r2, =__iwram_end
    bl      copy_words
    ldr     r0, =__data_lma
    ldr     r1, =__data_start
    ldr     r2, =__data_end
    bl      copy_words
    ldr     r0, =__ewram_lma
    ldr     r1, =__ewram_start
    ldr     r2, =__ewram_end
    bl      copy_words
    ldr     r0, =__bss_start
    ldr     r1, =__bss_end
    bl      clear_words
    ldr     r0, =__sbss_start
    ldr     r1, =__sbss_end
    bl      clear_words

    ldr     r0, =main
    mov     lr, pc
    bx      r0
hang:
    b       hang

@ r0 = source, r1 = destination, r2 = destination end
copy_words:
    cmp     r1, r2
    ldrlo   r3, [r0], #4
    strlo   r3, [r1], #4
    blo     copy_words
    bx      lr

@ r0 = start, r1 = end
clear_words:
    mov     r2, #0
1:  cmp     r0, r1
    strlo   r2, [r0], #4
    blo     1b
    bx      lr

    .pool

@ IRQ handler, runs from IWRAM in ARM mode. It acknowledges the interrupt,
@ flags it for the BIOS IntrWait calls and counts VBlanks (vbl_count), so
@ the main loop can keep the music on time after a slow frame; all game
@ work happens in the main loop after VBlankIntrWait.
    .section .iwram, "ax"
    .arm
    .align 2
    .global irq_handler
irq_handler:
    mov     r0, #0x04000000
    add     r0, r0, #0x200
    ldr     r1, [r0]                @ IE | IF << 16
    and     r1, r1, r1, lsr #16     @ IE & IF
    strh    r1, [r0, #2]            @ acknowledge in IF
    ldr     r2, =0x03007FF8         @ BIOS interrupt check flags
    ldrh    r3, [r2]
    orr     r3, r3, r1
    strh    r3, [r2]
    tst     r1, #1                  @ VBlank?
    ldrne   r2, =vbl_count
    ldrne   r3, [r2]
    addne   r3, r3, #1
    strne   r3, [r2]
    bx      lr
    .pool
