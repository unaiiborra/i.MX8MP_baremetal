OUTPUT_FORMAT("elf64-littleaarch64")
ENTRY(_start)

SECTIONS {
    . = 0x40000000;

    .text : ALIGN(16) {
        KEEP(*(.text.boot))
        *(.text*)
        . = ALIGN(16);
    }

    .rodata : ALIGN(16) {
        *(.rodata*)
        . = ALIGN(16);
    }

    .data : ALIGN(16) {
        *(.data*)
        . = ALIGN(16);
    }

    .bss : ALIGN(64) {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(64);
        __bss_end = .;
    }

    .stacks (NOLOAD) : ALIGN(4096) {
        /* EL1 */
        . = ALIGN(16);
        __stack_el1_bottom = .;
        . += 0x500000;
        __stack_el1_top = .;

        /* EL2 */
        . = ALIGN(16);
        __stack_el2_bottom = .;
        . += 0x500000;
        __stack_el2_top = .;
    }

}
