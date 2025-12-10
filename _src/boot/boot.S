.section .text.boot
.global _start

_start:
    nop
    // disable irq
    msr daifset, #0xf

    // Init EL2 stack
    ldr x0, =__stack_el2_top
    mov sp, x0
    
    // Init El1 stack
    ldr x0, =__stack_el1_top
    msr SP_EL1, x0

    bl _clean_bss

    bl kernel_entry

hang:
    wfe
    b hang