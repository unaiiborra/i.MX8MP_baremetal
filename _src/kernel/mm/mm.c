#include "kernel/mm/mm.h"

#include <boot/panic.h>
#include <frdm_imx8mp.h>

#include "./init/early_kalloc.h"
#include "arm/mmu/mmu.h"
#include "drivers/uart/uart.h"
#include "kernel/devices/drivers.h"
#include "lib/stdint.h"
#include "lib/stdmacros.h"
#include "lib/string.h"
#include "lib/unit/mem.h"
#include "mm_info.h"


static void* mmu_allocator(size_t bytes, size_t alignment)
{
    (void)alignment;

    return (void*)early_kalloc(bytes, "mmu alignment", false);
}


static void mmu_freer(void* addr)
{
    char buf[200];
    stdint_to_ascii((STDINT_UNION) {.uint64 = (uintptr)addr}, STDINT_UINT64, buf, 200,
                    STDINT_BASE_REPR_HEX);

    UART_puts(&UART2_DRIVER, buf);
    UART_puts(&UART2_DRIVER, "\n\r");


    return;
}


void mm_early_init()
{
    mm_info_init();

    // init early kalloc. Used by the next initialization stages
    early_kalloc_init();


    /* reserve memory for mmio */
    early_kalloc(MEM_GiB * 1, "mmio", true); // TODO: not hardcoded


    /* reserve memory for TF-A reserved zone */
    size_t tf_a_bytes = mm_info_kernel_start() - mm_info_ddr_start();
    ASSERT(tf_a_bytes % MMU_GRANULARITY_4KB == 0);
    early_kalloc(tf_a_bytes, "tf_a_protected", true);


    /* reserve memory for the text + bss + rodata + data + el2:1 stacks of the kernel */
    ASSERT(mm_info_kernel_size() % MMU_GRANULARITY_4KB == 0);
    early_kalloc(mm_info_kernel_size(), "kernel_static", true);


    mmu_handle h;
    mmu_init(&h, MMU_GRANULARITY_4KB, mmu_allocator, mmu_freer);


    mmu_cfg device_cfg = mmu_cfg_new(1, MMU_AP_EL0_NONE_EL1_RW, 0, false, 1, 0, 0, 0);
    mmu_map(h, 0, 0, MEM_GiB, device_cfg, NULL);

    mmu_cfg mem_cfg = mmu_cfg_new(0, MMU_AP_EL0_NONE_EL1_RW, 0, false, 1, 0, 0, 0);
    mmu_map(h, MEM_GiB, MEM_GiB, 4 * MEM_GiB, mem_cfg, NULL);

    mmu_activate(h, true, true, false);
    mmu_debug_dump(h);


    mm_init();
}

void mm_init()
{
    mmu_handle stress_test_h;
    mmu_cfg mem_cfg = mmu_cfg_new(0, MMU_AP_EL0_NONE_EL1_RW, 0, false, 1, 0, 0, 0);

    mmu_init(&stress_test_h, MMU_GRANULARITY_4KB, mmu_allocator, mmu_freer);
    mmu_stress_test(stress_test_h, mem_cfg, 0, MEM_GiB);
}
