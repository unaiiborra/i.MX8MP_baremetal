#pragma once

#include <boot/panic.h>
#include <lib/mem.h>
#include <lib/stdint.h>

#include "arm/mmu/mmu.h"
#include "mmu_types.h"


#define NULL_PD (mmu_hw_pd) {.v = 0}


static inline mmu_hw_pd mmu_tbl_get_pd(mmu_tbl tbl, size_t i)
{
    DEBUG_ASSERT(tbl.pds, "null provided table");

    return tbl.pds[i];
}


static inline uint64 mmu_granularity_shift(mmu_granularity g)
{
    switch (g) {
        case MMU_GRANULARITY_4KB:
            return 12;
        case MMU_GRANULARITY_16KB:
            return 14;
        case MMU_GRANULARITY_64KB:
            return 16;
#ifdef TEST
        default:
            PANIC("mmu_granularity_shift unhandled");
#endif
    }
    return 12;
}


static inline uint64 output_address_bit_n_(mmu_granularity g)
{
    const uint64 shift = mmu_granularity_shift(g);
    const uint64 pa_bit_n = 48 - shift;

    return pa_bit_n;
}


static inline uint64 output_address_mask_(mmu_granularity g)
{
    const uint64 shift = mmu_granularity_shift(g);
    const uint64 pa_bit_n = 48 - shift;

    return ((1ULL << pa_bit_n) - 1) << shift;
}


/*
    Page descriptor bit definitions
*/
#define MMU_PD_VALID_SHIFT 0
#define MMU_PD_VALID_WIDTH 1

#define MMU_PD_TYPE_SHIFT 1
#define MMU_PD_TYPE_WIDTH 1

#define MMU_PD_ATTR_INDEX_SHIFT 2
#define MMU_PD_ATTR_INDEX_WIDTH 3

#define MMU_PD_NS_SHIFT 5
#define MMU_PD_NS_WIDTH 1

#define MMU_PD_AP_SHIFT 6
#define MMU_PD_AP_WIDTH 2

#define MMU_PD_SH_SHIFT 8
#define MMU_PD_SH_WIDTH 2

#define MMU_PD_AF_SHIFT 10
#define MMU_PD_AF_WIDTH 1

#define MMU_PD_PXN_SHIFT 53
#define MMU_PD_PXN_WIDTH 1

#define MMU_PD_UXN_SHIFT 54
#define MMU_PD_UXN_WIDTH 1

#define MMU_PD_SW_SHIFT 55
#define MMU_PD_SW_WIDTH 4


/* helpers */
#define MMU_PD_BITS(width) ((1ULL << (width)) - 1)
#define MMU_PD_FIELD_MASK(shift, width) (MMU_PD_BITS(width) << (shift))


// getters
static inline bool pd_get_valid(const mmu_hw_pd pd)
{
    return (bool)((pd.v >> MMU_PD_VALID_SHIFT) & MMU_PD_BITS(MMU_PD_VALID_WIDTH));
}

static inline mmu_pd_type pd_get_type(const mmu_hw_pd pd)
{
    return (mmu_pd_type)((pd.v >> MMU_PD_TYPE_SHIFT) & MMU_PD_BITS(MMU_PD_TYPE_WIDTH));
}

static inline uint8 pd_get_attr_index(const mmu_hw_pd pd)
{
    return (uint8)((pd.v >> MMU_PD_ATTR_INDEX_SHIFT) & MMU_PD_BITS(MMU_PD_ATTR_INDEX_WIDTH));
}

static inline bool pd_get_non_secure(const mmu_hw_pd pd)
{
    return (bool)((pd.v >> MMU_PD_NS_SHIFT) & MMU_PD_BITS(MMU_PD_NS_WIDTH));
}

static inline mmu_access_permission pd_get_access_permissions(const mmu_hw_pd pd)
{
    return (mmu_access_permission)((pd.v >> MMU_PD_AP_SHIFT) & MMU_PD_BITS(MMU_PD_AP_WIDTH));
}

static inline mmu_shareability pd_get_shareability(const mmu_hw_pd pd)
{
    return (mmu_shareability)((pd.v >> MMU_PD_SH_SHIFT) & MMU_PD_BITS(MMU_PD_SH_WIDTH));
}

static inline bool pd_get_access_flag(const mmu_hw_pd pd)
{
    return (bool)((pd.v >> MMU_PD_AF_SHIFT) & MMU_PD_BITS(MMU_PD_AF_WIDTH));
}

static inline uint64 pd_get_output_address(const mmu_hw_pd pd, mmu_granularity g)
{
    return (pd.v & output_address_mask_(g));
}

static inline bool pd_get_privileged_execute_never(const mmu_hw_pd pd)
{
    return (bool)((pd.v >> MMU_PD_PXN_SHIFT) & MMU_PD_BITS(MMU_PD_PXN_WIDTH));
}

static inline bool pd_get_unprivileged_execute_never(const mmu_hw_pd pd)
{
    return (bool)((pd.v >> MMU_PD_UXN_SHIFT) & MMU_PD_BITS(MMU_PD_UXN_WIDTH));
}

static inline uint8 pd_get_software_defined(const mmu_hw_pd pd)
{
    return (uint8)((pd.v >> MMU_PD_SW_SHIFT) & MMU_PD_BITS(MMU_PD_SW_WIDTH));
}


// setters
static inline void pd_set_valid(mmu_hw_pd* pd, bool valid)
{
    pd->v &= ~MMU_PD_FIELD_MASK(MMU_PD_VALID_SHIFT, MMU_PD_VALID_WIDTH);
    pd->v |= ((uint64)valid << MMU_PD_VALID_SHIFT);
}

static inline void pd_set_type(mmu_hw_pd* pd, mmu_pd_type type)
{
    pd->v &= ~MMU_PD_FIELD_MASK(MMU_PD_TYPE_SHIFT, MMU_PD_TYPE_WIDTH);
    pd->v |= ((uint64)type << MMU_PD_TYPE_SHIFT);
}

static inline void pd_set_attr_index(mmu_hw_pd* pd, uint8 attr_index)
{
    pd->v &= ~MMU_PD_FIELD_MASK(MMU_PD_ATTR_INDEX_SHIFT, MMU_PD_ATTR_INDEX_WIDTH);
    pd->v |= ((uint64)attr_index & MMU_PD_BITS(MMU_PD_ATTR_INDEX_WIDTH)) << MMU_PD_ATTR_INDEX_SHIFT;
}

static inline void pd_set_non_secure(mmu_hw_pd* pd, bool non_secure)
{
    pd->v &= ~MMU_PD_FIELD_MASK(MMU_PD_NS_SHIFT, MMU_PD_NS_WIDTH);
    pd->v |= ((uint64)non_secure << MMU_PD_NS_SHIFT);
}

static inline void pd_set_access_permissions(mmu_hw_pd* pd, mmu_access_permission permissions)
{
    pd->v &= ~MMU_PD_FIELD_MASK(MMU_PD_AP_SHIFT, MMU_PD_AP_WIDTH);
    pd->v |= ((uint64)permissions << MMU_PD_AP_SHIFT);
}

static inline void pd_set_shareability(mmu_hw_pd* pd, mmu_shareability shareability)
{
    pd->v &= ~MMU_PD_FIELD_MASK(MMU_PD_SH_SHIFT, MMU_PD_SH_WIDTH);
    pd->v |= ((uint64)shareability & MMU_PD_BITS(MMU_PD_SH_WIDTH)) << MMU_PD_SH_SHIFT;
}

static inline void pd_set_access_flag(mmu_hw_pd* pd, bool access_flag)
{
    pd->v &= ~MMU_PD_FIELD_MASK(MMU_PD_AF_SHIFT, MMU_PD_AF_WIDTH);
    pd->v |= ((uint64)access_flag << MMU_PD_AF_SHIFT);
}

static inline void pd_set_output_address(mmu_hw_pd* pd, uint64 output_address,
                                         mmu_granularity granularity)
{
    const uint64 pa_bit_n = output_address_bit_n_(granularity);

    if (output_address >= (1ULL << pa_bit_n)) {
#ifdef TEST
        PANIC("pd_set_output_address: invalid output address, out of granularity");
#endif
        return;
    }

    if (output_address % granularity != 0) {
#ifdef TEST
        PANIC("pd_set_output_address: invalid output address, not aligned to granularity");
#endif
        return;
    }

    const uint64 mask = output_address_mask_(granularity);

    pd->v &= ~mask;
    pd->v |= output_address & mask;
}


static inline void pd_set_privileged_execute_never(mmu_hw_pd* pd, bool pxn)
{
    pd->v &= ~MMU_PD_FIELD_MASK(MMU_PD_PXN_SHIFT, MMU_PD_PXN_WIDTH);
    pd->v |= ((uint64)pxn << MMU_PD_PXN_SHIFT);
}

static inline void pd_set_unprivileged_execute_never(mmu_hw_pd* pd, bool uxn)
{
    pd->v &= ~MMU_PD_FIELD_MASK(MMU_PD_UXN_SHIFT, MMU_PD_UXN_WIDTH);
    pd->v |= ((uint64)uxn << MMU_PD_UXN_SHIFT);
}

static inline void pd_set_software_defined(mmu_hw_pd* pd, uint8 software_defined)
{
    pd->v &= ~MMU_PD_FIELD_MASK(MMU_PD_SW_SHIFT, MMU_PD_SW_WIDTH);
    pd->v |= ((uint64)software_defined & MMU_PD_BITS(MMU_PD_SW_WIDTH)) << MMU_PD_SW_SHIFT;
}


static inline mmu_hw_pd td_build(mmu_tbl next, mmu_granularity g)
{
    mmu_hw_pd pd = (mmu_hw_pd) {0};

    pd_set_type(&pd, MMU_PD_TABLE);
    pd_set_output_address(&pd, (uintptr)next.pds, g);
    pd_set_valid(&pd, true);

    return pd;
}


static inline mmu_hw_pd bd_build(mmu_pg_cfg cfg, p_uintptr output_address, mmu_granularity g)
{
    mmu_hw_pd pd = (mmu_hw_pd) {0};

    pd_set_valid(&pd, true);
    pd_set_type(&pd, MMU_PD_BLOCK);
    pd_set_attr_index(&pd, cfg.attr_index);
    pd_set_non_secure(&pd, cfg.non_secure);
    pd_set_access_permissions(&pd, cfg.ap);
    pd_set_shareability(&pd, cfg.shareability);
    pd_set_access_flag(&pd, cfg.access_flag);

    pd_set_output_address(&pd, output_address, g);

    pd_set_privileged_execute_never(&pd, cfg.pxn);
    pd_set_unprivileged_execute_never(&pd, cfg.uxn);
    pd_set_software_defined(&pd, cfg.sw);

    return pd;
}