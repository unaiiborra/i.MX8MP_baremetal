#include "identity_mapping.h"

#include <lib/stdint.h>
#include <lib/stdmacros.h>

#include "./early_kalloc.h"
#include "arm/mmu/mmu.h"
#include "boot/panic.h"
#include "lib/mem.h"


static void* im_alloc(size_t bytes, size_t alignment)
{
    p_uintptr v = early_kalloc(bytes, "mm_early_identity_mapping_tbl", false, false);

    ASSERT(v % alignment == 0);

    return (void*)v;
}

static void im_free(void* addr)
{
#ifdef DEBUG
    char buf[200];
    stdint_to_ascii((STDINT_UNION) {.uint64 = (uintptr)addr}, STDINT_UINT64, buf, 200,
                    STDINT_BASE_REPR_HEX);

    uart_puts(&UART2_DRIVER, buf);
    uart_puts(&UART2_DRIVER, "\n\r");
#else
    PANIC("The early identity mapping allocations should not free any tables");
    (void)addr;
#endif
}


mmu_handle early_identity_mapping()
{
    mmu_cfg cfg = (mmu_cfg) {
        .enable = false,
        .d_cache = false,
        .i_cache = false,
        .align_trap = true,

        .hi_enable = false,
        .lo_enable = true,

        .hi_va_addr_bits = 48,
        .lo_va_addr_bits = 48,

        .hi_gran = MMU_GRANULARITY_4KB,
        .lo_gran = MMU_GRANULARITY_4KB,
    };

    mmu_handle h;
    mmu_init(&h, cfg, im_alloc, im_free);


    mmu_pg_cfg device_cfg = mmu_pg_cfg_new(1, MMU_AP_EL0_NONE_EL1_RW, 0, false, 1, 0, 0, 0);
    mmu_map(&h, 0, 0, MEM_GiB, device_cfg, NULL);

    mmu_pg_cfg mem_cfg = mmu_pg_cfg_new(0, MMU_AP_EL0_NONE_EL1_RW, 0, false, 1, 0, 0, 0);
    mmu_map(&h, MEM_GiB, MEM_GiB, 4 * MEM_GiB, mem_cfg, NULL);


    cfg.enable = true;
    cfg.d_cache = true;
    cfg.i_cache = true;
    cfg.align_trap = false;
    mmu_reconfig(&h, cfg, NULL, NULL);

    return h;
}