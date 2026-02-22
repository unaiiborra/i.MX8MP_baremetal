#include <arm/exceptions/exceptions.h>
#include <arm/tfa/smccc.h>
#include <drivers/arm_generic_timer/arm_generic_timer.h>
#include <drivers/interrupts/gicv3/gicv3.h>
#include <drivers/tmu/tmu.h>
#include <kernel/init.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/stdint.h>
#include <lib/stdmacros.h>
#include <lib/string.h>

#include "arm/cpu.h"
#include "kernel/io/term.h"
#include "mm/mm_info.h"
#include "mm/phys/page_allocator.h"


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

#define N 1'000'000
#define M 200

    void** test = kmalloc(sizeof(void*) * N);
    void** test2 = kmalloc(sizeof(void*) * M);


    for (size_t i = 0; i < N; i++) {
        test[i] = cache_malloc(CACHE_32);
        if (i % (N / 100) == 0)
            term_printf("cache_malloc(CACHE_32): %d / %d\n\r", i, N);
    }

    for (size_t i = 0; i < N; i++) {
        cache_free(CACHE_32, test[i]);
        if (i % (N / 100) == 0)
            term_printf("cache_free(CACHE_32, test[i]): %d / %d\n\r", i, N);
    }

    for (size_t i = 0; i < N; i++) {
        test[i] = kmalloc(9);
        if (i % (N / 100) == 0)
            term_printf("kmalloc(9): %d / %d\n\r", i, N);
    }

    for (size_t j = 0; j < M; j++) {
        test2[j] = kmalloc(2 * KPAGE_SIZE);
    }

    for (size_t i = 0; i < N; i++) {
        kfree(test[i]);
        if (i % (N / 100) == 0)
            term_printf("kfree(test[i]): %d / %d\n\r", i, N);
    }

    for (size_t j = 0; j < M; j++) {
        kfree(test2[j]);
    }


    term_prints("->>>>>>>>>\n\r");
    page_allocator_debug();


    loop asm volatile("wfi");
}
