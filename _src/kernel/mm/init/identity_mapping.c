#include "identity_mapping.h"

#include <arm/mmu/mmu.h>
#include <kernel/io/stdio.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/mem.h>
#include <lib/stdint.h>
#include <lib/stdmacros.h>
#include <lib/string.h>

#include "../init/mem_regions/early_kalloc.h"
#include "../mm_info.h"
#include "mem_regions/mem_regions.h"


/// returns the pa, not va, but as when relocating the address will be relocated, works fine.
/// Providing the va at this stage cannot work as it is the allocator for initializing the first
/// identity mapping without the mmu still enabled
static void* im_alloc(size_t bytes, size_t alignment)
{
    p_uintptr v = early_kalloc(bytes, "mmu table", false, false).pa;

    ASSERT(v % alignment == 0);

    return (void*)v;
}


static void im_free(void* addr)
{
    char buf[200];
    stdint_to_ascii((STDINT_UNION) {.uint64 = (v_uintptr)addr}, STDINT_UINT64, buf, 200,
                    STDINT_BASE_REPR_HEX);

    kprintf("%s\n\r", buf);

#ifndef DEBUG
    PANIC("The early identity mapping allocations should not free any tables");
#endif
}


void early_identity_mapping()
{
    mmu_cfg cfg;
    mmu_cfg_new(&cfg, true, true, false, true, true, 48, 48, MMU_GRANULARITY_4KB,
                MMU_GRANULARITY_4KB);

    mmu_init(&mm_mmu_h, cfg, im_alloc, im_free, 0);


    mmu_pg_cfg device_cfg = mmu_pg_cfg_new(1, MMU_AP_EL0_NONE_EL1_RW, 0, false, 1, 0, 0, 0);
    mmu_pg_cfg mem_cfg = mmu_pg_cfg_new(0, MMU_AP_EL0_NONE_EL1_RW, 0, false, 1, 0, 0, 0);


    for (size_t i = 0; i < MEM_REGIONS.REG_COUNT; i++) {
        const mem_region r = mm_as_kpa_ptr(MEM_REGIONS.REGIONS)[i];

        mmu_pg_cfg* cfg;
        switch (r.type) {
            case MEM_REGION_DDR:
                cfg = &mem_cfg;
                break;
            case MEM_REGION_MMIO:
                cfg = &device_cfg;
                break;
            default:
                PANIC();
                break;
        }

        bool result = mmu_map(&mm_mmu_h, r.start, r.start, r.size, *cfg, NULL);
        ASSERT(result);

        result = mmu_map(&mm_mmu_h, mm_as_kva(r.start), r.start, r.size, *cfg, NULL);
        ASSERT(result);
    }


    mmu_activate();
}