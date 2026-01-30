#pragma once
#include <lib/math.h>

#include "arm/mmu/mmu.h"
#include "boot/panic.h"
#include "lib/lock/_lock_types.h"
#include "lib/mem.h"
#include "lib/stdint.h"
#include "mmu_pd.h"
#include "mmu_types.h"


static inline size_t level_shift_(mmu_granularity g, mmu_tbl_level l)
{
    size_t page_bits = log2_floor_u64(g); // 12 / 14 / 16
    size_t index_bits = page_bits - 3;

    size_t max_level = (g == MMU_GRANULARITY_64KB) ? MMU_TBL_LV2 : MMU_TBL_LV3;

    ASSERT(l <= max_level);

    return page_bits + index_bits * (max_level - l);
}


static inline mmu_tbl_level max_level(mmu_granularity g)
{
    return g == MMU_GRANULARITY_64KB ? MMU_TBL_LV2 : MMU_TBL_LV3;
}


static inline size_t tbl_entries(mmu_granularity g)
{
    return g / sizeof(mmu_hw_pd);
}

static inline uint64 tbl_alignment(mmu_granularity g)
{
    return g;
}


static inline mmu_tbl tbl0_from_handle(mmu_handle* h)
{
    DEBUG_ASSERT(h->tbl0_ && ((p_uintptr)h->tbl0_ % tbl_alignment(h->cfg_.lo_gran) == 0));

    return (mmu_tbl) {.pds = h->tbl0_};
}


static inline mmu_tbl tbl1_from_handle(mmu_handle* h)
{
    DEBUG_ASSERT(h->tbl1_ && ((p_uintptr)h->tbl1_ % tbl_alignment(h->cfg_.hi_gran) == 0));

    return (mmu_tbl) {.pds = h->tbl1_};
}

static inline mmu_tbl tbl_from_ptr(uintptr addr, mmu_granularity g)
{
    ASSERT(addr && addr % g == 0);

    return (mmu_tbl) {.pds = (mmu_hw_pd*)addr};
}

static inline mmu_tbl tbl_from_td(mmu_hw_pd pd, mmu_granularity g)
{
    ASSERT(pd_get_type(pd) == MMU_PD_TABLE);

    return tbl_from_ptr(pd_get_output_address(pd, g), g);
}


static inline size_t table_index(p_uintptr va, mmu_granularity g, mmu_tbl_level l)
{
    size_t index_bits = log2_floor_u64(g) - 3;
    size_t mask = (1ULL << index_bits) - 1;

    return (va >> level_shift_(g, l)) & mask;
}


static inline size_t pd_cover_bytes(mmu_granularity g, mmu_tbl_level l)
{
    size_t page_bits = log2_floor_u64(g);
    size_t index_bits = page_bits - 3;

    ASSERT(l <= max_level(g));

    return 1ULL << (page_bits + index_bits * (max_level(g) - l));
}


static inline void tbl_init_null(mmu_tbl tbl, mmu_granularity g)
{
    size_t entries = tbl_entries(g);

    for (size_t i = 0; i < entries; i++)
        tbl.pds[i].v = 0;
}


static inline mmu_tbl alloc_tbl(mmu_alloc alloc, mmu_granularity g, bool init_null,
                                mmu_op_info* info)
{
    void* addr = alloc(sizeof(mmu_hw_pd) * tbl_entries(g), tbl_alignment(g));

    if (info)
        info->alocated_tbls++;

    ASSERT((uintptr)addr % tbl_alignment(g) == 0);


    mmu_tbl tbl = (mmu_tbl) {.pds = addr};

    if (init_null)
        tbl_init_null(tbl, g);


    return tbl;
}


static inline mmu_pg_cfg cfg_from_pd(mmu_hw_pd pd)
{
    return (mmu_pg_cfg) {
        .attr_index = pd_get_attr_index(pd),
        .ap = pd_get_access_permissions(pd),
        .shareability = pd_get_shareability(pd),
        .non_secure = pd_get_non_secure(pd),
        .access_flag = pd_get_access_flag(pd),
        .pxn = pd_get_privileged_execute_never(pd),
        .uxn = pd_get_unprivileged_execute_never(pd),
        .sw = pd_get_software_defined(pd),
    };
}

/// divides a block into a next level table and updates the parent. Returns the created table (of a
/// lower level)
static inline mmu_tbl split_block(mmu_handle* h, mmu_tbl parent, size_t index, mmu_tbl_level l,
                                  mmu_op_info* info)
{
    mmu_granularity g = h->cfg_.lo_gran;
    mmu_alloc alloc = h->alloc_;

    mmu_hw_pd old = parent.pds[index];
    mmu_pg_cfg cfg = cfg_from_pd(old);
    p_uintptr pa = pd_get_output_address(old, g);
    size_t new_l_bytes = pd_cover_bytes(g, l + 1);

    ASSERT(l < max_level(g));
    ASSERT(pd_get_type(old) == MMU_PD_BLOCK);
    ASSERT(pa % pd_cover_bytes(g, l) == 0);


    mmu_tbl new_tbl = alloc_tbl(alloc, g, false, info);

    // create the new blocks
    for (size_t i = 0; i < tbl_entries(g); i++)
        new_tbl.pds[i] = bd_build(cfg, pa + (i * new_l_bytes), g);

    // set the new table
    parent.pds[index] = td_build(new_tbl, g);

    return new_tbl;
}


static inline bool tbl_is_null(mmu_tbl tbl, mmu_granularity g)
{
    bool r = true;
    for (size_t i = 0; i < tbl_entries(g); i++)
        if (pd_get_valid(tbl.pds[i])) {
            r = false;
            break;
        }

    return r;
}


static inline bool va_uses_ttbr0(mmu_handle* h, v_uintptr va)
{
    uint64 mask = ~((1ULL << h->cfg_.hi_va_addr_bits) - 1);
    return (va & mask) != mask;
}

static inline mmu_tbl get_first_tbl(mmu_handle* h, v_uintptr va)
{
    return va_uses_ttbr0(h, va) ? tbl0_from_handle(h) : tbl1_from_handle(h);
}

static inline mmu_granularity get_granularity(mmu_handle* h, v_uintptr va)
{
    return va_uses_ttbr0(h, va) ? h->cfg_.lo_gran : h->cfg_.hi_gran;
}
