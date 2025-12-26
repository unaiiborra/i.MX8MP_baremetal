// https://github.com/antmicro/u-boot/blob/master/arch/arm/cpu/armv8/cache_v8.c#L133

.section .text
.align 4

.global _switch_to_el1
_switch_to_el1:
    mrs x0, CurrentEL
    lsr x0, x0, #2
    cmp x0, #2
    b.ne hang

    ldr x0, =__stack_el1_top
    msr SP_EL1, x0

    /* EL1 AArch64 */
    mov x0, #(1 << 31)
    msr HCR_EL2, x0

    adr x0, _el1_vector_table
    msr VBAR_EL1, x0

    mov x0, #0x3c5
    msr SPSR_EL2, x0

    // Enable simd instructions for EL1
    mov  x0, #(3 << 20)    // FPEN = 0b11
    msr  CPACR_EL1, x0

    ldr x0, =_el1_entry
    msr ELR_EL2, x0

    ldr x0, =__text_start
    ldr x1, =__bss_end
    bl _cache_flush_range

    dsb sy
    dsb ish
    isb

    eret

hang:
    wfe
    b hang


.align 4
.global _el1_entry
_el1_entry:
    nop
    mov x0, #1
    msr spsel, x0

    mov x0, #0x00            
    msr MAIR_EL1, x0

    mov x0, #(16)            
    msr TCR_EL1, x0

    mov x0, #(1 << 11)      
    msr SCTLR_EL1, x0
    isb

    bl kernel_entry
    