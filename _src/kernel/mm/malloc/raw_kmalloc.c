#include "raw_kmalloc.h"

#include <arm/mmu/mmu.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/lock/corelock.h>
#include <lib/math.h>
#include <lib/mem.h>
#include <lib/stdbool.h>
#include <lib/stdint.h>
#include <lib/stdmacros.h>

#include "../mm_info.h"
#include "../phys/page.h"
#include "../phys/page_allocator.h"
#include "../virt/vmalloc.h"
#include "reserve_malloc.h"


const raw_kmalloc_cfg RAW_KMALLOC_DEFAULT_CFG = (raw_kmalloc_cfg) {
    .assign_pa = true,
    .fill_reserve = true,
    .device_mem = false,
    .permanent = false,
    .kmap = false,
};


static const mmu_pg_cfg STD_MMU_KMEM_CFG = (mmu_pg_cfg) {
    .attr_index = 0,
    .ap = MMU_AP_EL0_NONE_EL1_RW,
    .shareability = 0,
    .non_secure = false,
    .access_flag = 1,
    .pxn = 0,
    .uxn = 0,
    .sw = 0,
};

static const mmu_pg_cfg STD_MMU_DEVICE_CFG = (mmu_pg_cfg) {
    .attr_index = 1,
    .ap = MMU_AP_EL0_NONE_EL1_RW,
    .shareability = 0,
    .non_secure = false,
    .access_flag = 1,
    .pxn = 0,
    .uxn = 0,
    .sw = 0,
};


static corelock_t lock;


static inline vmalloc_cfg vmalloc_cfg_from_raw_kmalloc_cfg(const raw_kmalloc_cfg* cfg,
                                                           p_uintptr kmap_pa)
{
    return (vmalloc_cfg) {
        .assing_pa = cfg->assign_pa,
        .device_mem = cfg->device_mem,
        .permanent = cfg->permanent,
        .kmap =
            {
                .use_kmap = cfg->kmap,
                .kmap_pa = cfg->kmap ? kmap_pa : 0,
            },
    };
}


static void* raw_kmalloc_kmap(size_t pages, const char* tag, const raw_kmalloc_cfg* cfg)
{
    DEBUG_ASSERT(cfg->kmap && cfg->assign_pa);

    ASSERT(is_pow2(pages), "only pow2 n pages can be kmapped");

    size_t o = log2_floor(pages);

    mm_page page = page_malloc(o, (mm_page_data) {
                                      .tag = tag,
                                      .device_mem = cfg->device_mem,
                                      .permanent = cfg->permanent,
                                  });

    v_uintptr va = vmalloc(pages, tag, vmalloc_cfg_from_raw_kmalloc_cfg(cfg, page.pa));

    DEBUG_ASSERT(ptrs_are_kmapped(pv_ptr_new(page.pa, va)));

    const mmu_pg_cfg* mmu_cfg = cfg->device_mem ? &STD_MMU_DEVICE_CFG : &STD_MMU_KMEM_CFG;
    bool mmu_res = mmu_map(&mm_mmu_h, va, page.pa, pages * KPAGE_SIZE, *mmu_cfg, NULL);
    ASSERT(mmu_res);

    return (void*)va;
}


static void* raw_kmalloc_dynamic(size_t pages, const char* tag, const raw_kmalloc_cfg* cfg)
{
    DEBUG_ASSERT(!cfg->kmap);
    ASSERT(cfg->assign_pa, "vmalloc: TODO: NOT IMPLEMENTED YET");


    v_uintptr start = vmalloc(pages, tag, vmalloc_cfg_from_raw_kmalloc_cfg(cfg, 0));

    v_uintptr v = start;
    size_t rem = pages;

    while (rem > 0) {
        size_t o = log2_floor(rem);
        size_t order_bytes = power_of2(o) * KPAGE_SIZE;

        mm_page page = page_malloc(o, (mm_page_data) {
                                          .tag = tag,
                                          .device_mem = cfg->device_mem,
                                          .permanent = cfg->permanent,
                                      });

        const mmu_pg_cfg* mmu_cfg = cfg->device_mem ? &STD_MMU_DEVICE_CFG : &STD_MMU_KMEM_CFG;
        bool mmu_res = mmu_map(&mm_mmu_h, v, page.pa, order_bytes, *mmu_cfg, NULL);
        ASSERT(mmu_res);


        v += order_bytes;
        rem -= power_of2(o);
    }

    DEBUG_ASSERT(start + (pages * KPAGE_SIZE) == v);

    return (void*)start;
}

void raw_kmalloc_init()
{
    corelock_init(&lock);
}


void* raw_kmalloc(size_t pages, const char* tag, const raw_kmalloc_cfg* cfg)
{
    void* va;

    cfg = (cfg != NULL) ? cfg : &RAW_KMALLOC_DEFAULT_CFG;

    ASSERT(cfg->assign_pa, "TODO: dynamic mapping not implemented yet");

    corelocked(&lock)
    {
        if (cfg->kmap) {
            va = raw_kmalloc_kmap(pages, tag, cfg);
        }
        else {
            va = raw_kmalloc_dynamic(pages, tag, cfg);
        }

        if (cfg->fill_reserve)
            reserve_malloc_fill();
    }

    return va;
}


void raw_kfree(void* ptr)
{
    (void)ptr;
    corelocked(&lock)
    {
    }
}