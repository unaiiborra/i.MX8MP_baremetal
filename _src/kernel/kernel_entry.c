#include <arm/exceptions/exceptions.h>
#include <arm/tfa/smccc.h>
#include <drivers/arm_generic_timer/arm_generic_timer.h>
#include <drivers/interrupts/gicv3/gicv3.h>
#include <drivers/tmu/tmu.h>
#include <kernel/init.h>
#include <kernel/panic.h>
#include <lib/stdint.h>
#include <lib/stdmacros.h>
#include <lib/string.h>

#include "arm/cpu.h"
#include "kernel/io/term.h"
#include "kernel/mm.h"
#include "lib/mem.h"
#include "lib/stdbool.h"
#include "mm/mm_info.h"
#include "mm/phys/page_allocator.h"
#include "mm/virt/vmalloc.h"


// Main function of the kernel, called by the bootloader (/boot/boot.S)
_Noreturn void kernel_entry()
{
    size_t coreid = ARM_get_cpu_affinity().aff0;
    if (coreid == 0) {
        if (!mm_kernel_is_relocated()) {
            kernel_early_init();
        }
        else {
            kernel_init();
        }
    }

    __attribute((unused)) mm_ksections x = MM_KSECTIONS;

    for (size_t i = 0; i < 100; i++) {
        size_t j = 100 - i;

        term_printf("i=%d j=%d\n\r", i, j);

        __attribute((unused)) void* b = raw_kmalloc(j, "test", NULL);

        vmalloc_va_info i = vmalloc_get_addr_info(b);

        ASSERT(i.state == VMALLOC_VA_INFO_RESERVED &&
               i.state_info.reserved.reserved_start == (v_uintptr)b &&
               i.state_info.reserved.reserved_size == j * KPAGE_SIZE);
    }


    page_allocator_debug();

    __attribute((unused)) vmalloc_va_info i = vmalloc_get_addr_info((void*)(~(uint64)0 - 0x4000));

    vmalloc_debug_free();
    vmalloc_debug_reserved();


    term_prints("end\n\r");

    loop asm volatile("wfi");
}
