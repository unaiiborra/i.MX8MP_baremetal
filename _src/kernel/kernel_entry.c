#include <arm/exceptions/exceptions.h>
#include <arm/tfa/smccc.h>
#include <drivers/arm_generic_timer/arm_generic_timer.h>
#include <drivers/interrupts/gicv3/gicv3.h>
#include <drivers/tmu/tmu.h>
#include <kernel/init.h>
#include <kernel/lib/kvec.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/stdint.h>
#include <lib/stdmacros.h>
#include <lib/string.h>

#include "arm/cpu.h"
#include "kernel/io/term.h"
#include "mm/mm_info.h"


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

    __attribute((unused)) mm_ksections y = MM_KSECTIONS;


    term_printf("\n\rSTART\n\r");


    kvec k = kvec_new(uint64);

#define N 50'000'000
    for (size_t i = 0; i < N; i++) {
        kvec_push(&k, &i);
    }

    uint64 x;
    uint64 value = 0xFFFULL;

    for (size_t i = 0; i < kvec_len(k); i++) {
        bool result = kvec_get(&k, i, &x);

        ASSERT(result && x == i);
    }


    term_printf("\n\rEND0\n\r");

    for (size_t i = 0; i < kvec_len(k); i++) {
        bool result = kvec_set(&k, i, &value, &x);
        ASSERT(result && x == i);
    }

    for (size_t i = 0; i < kvec_len(k); i++) {
        ASSERT(kvec_get(&k, i, &x));
        ASSERT(x == value);
    }


    term_printf("\n\rEND1\n\r");


    loop asm volatile("wfi");
}
