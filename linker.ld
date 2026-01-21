OUTPUT_FORMAT("elf64-littleaarch64")
ENTRY(_start)

__NUM_CORES             = 4;
__EL2_STACK_SIZE        = 0x20000;  /*  1/8MiB   */ 
__EL1_STACK_SIZE        = 0x100000; /*  1MiB     */
__TFA_PROTECTED_SIZE    = 0x200000;


INCLUDE memory.ld
INCLUDE symbols.ld


SECTIONS {
    . = ORIGIN(DDR);

    .text ORIGIN(DDR) + __TFA_PROTECTED_SIZE : ALIGN(64) {
        __text_start = .;
        KEEP(*(.text.boot))
        *(.text*)
        . = ALIGN(64);
        __text_end = .;
    } > DDR

    .vectors : ALIGN(2048) {
        __vectors_start = .;
        KEEP(*(.vectors))
        . = ALIGN(64);
        __vectors_end = .;
    } > DDR

    .rodata : ALIGN(16) {
        *(.rodata*)
        *(.rodata.*)
        *(.platform_info)

        /* Stage 0 */
        . = ALIGN(8);
        __kernel_init_stage0_start = .;
        KEEP(*(.kernel_init_stage0))
        __kernel_init_stage0_end = .;

        /* Stage 1 */
        . = ALIGN(8);
        __kernel_init_stage1_start = .;
        KEEP(*(.kernel_init_stage1))
        __kernel_init_stage1_end = .;

        /* Stage 2 */
        . = ALIGN(8);
        __kernel_init_stage2_start = .;
        KEEP(*(.kernel_init_stage2))
        __kernel_init_stage2_end = .;

        . = ALIGN(16);
    } > DDR

    .data : ALIGN(16) {
        *(.data*)
        . = ALIGN(16);
    } > DDR

    .bss : ALIGN(64) {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(64);
        __bss_end = .;
    } > DDR

    .stacks (NOLOAD) : ALIGN(4096) {
        __stacks_el2_start = .;
        . += (__NUM_CORES * __EL2_STACK_SIZE);
        __stacks_el2_end = .;
        . = ALIGN(16);
        __stacks_el1_start = .;
        . += (__NUM_CORES * __EL1_STACK_SIZE);
        __stacks_el1_end = .;
    } > DDR

    /* aligned to 4MiB pages, 1GiB size */
    .kernel_mem (NOLOAD) : ALIGN(0x1000 * 4) {
        __kernel_mem_start = .;
        . += 0x40000000;
        __kernel_mem_end = .;
    } > DDR
}
