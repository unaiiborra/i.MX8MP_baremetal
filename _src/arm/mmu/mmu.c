#include "arm/mmu/mmu.h"

#include <lib/lock/spinlock.h>
#include <lib/math.h>
#include <lib/mem.h>
#include <lib/stdint.h>
#include <lib/stdmacros.h>

#include "boot/panic.h"
#include "lib/lock/spinlock_irq.h"
#include "mmu_helpers.h"
#include "mmu_pd.h"
#include "mmu_types.h"
#include "regs/mmu_sysregs.h"

static void mmu_apply(mmu_handle* h);

void mmu_init(mmu_handle* h, mmu_cfg cfg, mmu_alloc alloc, mmu_free free)
{
    mmu_tbl tbl0 = alloc_tbl(alloc, cfg.lo_gran, true, NULL);
    mmu_tbl tbl1 = alloc_tbl(alloc, cfg.hi_gran, true, NULL);

    *h = (mmu_handle) {

        .alloc_ = alloc,
        .free_ = free,
        .tbl0_ = (void*)tbl0.pds,
        .tbl1_ = (void*)tbl1.pds,
        .cfg_ = cfg,
        .slock_ = {0},
    };

    spinlock_init(&h->slock_);

    mmu_apply(h);
}

mmu_cfg mmu_get_cfg(mmu_handle* h)
{
    return h->cfg_;
}

void mmu_reconfig(mmu_handle* h, mmu_cfg cfg, mmu_alloc alloc, mmu_free free)
{
    h->alloc_ = alloc == NULL ? h->alloc_ : alloc;
    h->free_ = free == NULL ? h->free_ : free;
    h->cfg_ = cfg;

    mmu_apply(h); // is already locked
}


mmu_pg_cfg mmu_pg_cfg_new(uint8 attr_index, mmu_access_permission ap, uint8 shareability,
                          bool non_secure, bool access_flag, bool pxn, bool uxn, uint8 sw)
{
    return (mmu_pg_cfg) {
        .attr_index = attr_index,
        .ap = ap,
        .shareability = shareability,
        .non_secure = non_secure,
        .access_flag = access_flag,
        .pxn = pxn,
        .uxn = uxn,
        .sw = sw,
    };
}


static void UNLOCKED_free_subtree(mmu_handle* h, mmu_tbl tbl, mmu_op_info* info)
{
    mmu_granularity g = h->cfg_.lo_gran;

    for (size_t i = 0; i < tbl_entries(g); i++)
        if (pd_get_valid(tbl.pds[i]) && pd_get_type(tbl.pds[i]) == MMU_PD_TABLE)
            UNLOCKED_free_subtree(h, tbl_from_td(tbl.pds[i], g), info);


    h->free_(tbl.pds);

    if (info)
        info->freed_tbls++;
}


static inline void get_target_lvl(mmu_tbl_level* target_lvl, size_t* cover, size_t size,
                                  mmu_granularity g, v_uintptr virt, p_uintptr phys)
{
    mmu_tbl_level l;
    size_t c;
    for (l = MMU_TBL_LV1; l <= max_level(g); l++) {
        c = pd_cover_bytes(g, l);

        if (size >= pd_cover_bytes(g, l) && virt % c == 0 && phys % c == 0)
            break;
    }

    if (target_lvl)
        *target_lvl = l;
    if (cover)
        *cover = c;
}


static bool UNLOCKED_map_(mmu_handle* h, v_uintptr virt, p_uintptr phys, size_t size,
                          mmu_pg_cfg cfg, mmu_op_info* info)
{
#ifdef DEBUG
    if (info)
        info->iters += 1;

    p_uintptr expected_phys_end = phys + size;
    v_uintptr expected_virt_end = virt + size;
#endif

    if (size == 0)
        return true;


    mmu_granularity g = h->cfg_.lo_gran;
    mmu_tbl tbl0 = tbl0_from_handle(h);
    mmu_alloc alloc = h->alloc_;

    mmu_tbl_level target_lvl;
    size_t cover;

    if (size % g != 0 || virt % g != 0 || phys % g != 0)
        return false;


    while (size > 0) {
        get_target_lvl(&target_lvl, &cover, size, g, virt, phys);


        DEBUG_ASSERT(size % g == 0 || virt % g == 0 || phys % g == 0);
        DEBUG_ASSERT(virt % cover == 0);
        DEBUG_ASSERT(phys % cover == 0);


        // the main loop of the fn, finds the actual table of the target level

        size_t i;
        mmu_tbl tbl = tbl0;

        for (mmu_tbl_level l = MMU_TBL_LV0; l < target_lvl; l++) {
            i = table_index(virt, g, l);

            mmu_hw_pd pd = mmu_tbl_get_pd(tbl, i);

            // not valid
            if (!pd_get_valid(pd)) {
                // --- invalid descriptor ---
                // allocate a new table
                mmu_tbl next = alloc_tbl(alloc, g, true, info);

                // link the new allocated table in the current table level
                tbl.pds[i] = td_build(next, g);
                tbl = next;

                continue;
            }

            switch (pd_get_type(pd)) {
                case MMU_PD_BLOCK:
                    tbl = split_block(h, tbl, i, l, info);
                    continue;
                case MMU_PD_TABLE:
                    tbl = tbl_from_ptr(pd_get_output_address(pd, g), g);
                    continue;
                default:
                    PANIC("mmu_map: err");
            }
        }

        // build the block descriptor
        i = table_index(virt, g, target_lvl);

        mmu_hw_pd old = mmu_tbl_get_pd(tbl, i);

        tbl.pds[i] = bd_build(cfg, phys, g);

        // if it was a table, free it (and all the subtables)
        if (pd_get_valid(old) && pd_get_type(old) == MMU_PD_TABLE)
            UNLOCKED_free_subtree(h, tbl_from_td(old, g), info);


        size -= cover;
        phys += cover;
        virt += cover;
    }

#ifdef DEBUG
    DEBUG_ASSERT(size == 0 && phys == expected_phys_end && virt == expected_virt_end);
#endif

    return true;
}


bool mmu_map(mmu_handle* h, v_uintptr virt, p_uintptr phys, size_t size, mmu_pg_cfg cfg,
             mmu_op_info* info)
{
    bool result;

    if (info)
        *info = (mmu_op_info) {
#ifdef DEBUG
            .iters = 0,
#endif
            .alocated_tbls = 0,
            .freed_tbls = 0,
        };

    spin_lock(&h->slock_);
    result = UNLOCKED_map_(h, virt, phys, size, cfg, info);
    spin_unlock(&h->slock_);

    return result;
}


bool UNLOCKED_unmap_(mmu_handle* h, v_uintptr virt, size_t size, mmu_op_info* info)
{
#ifdef DEBUG
    if (info)
        info->iters += 1;

    v_uintptr expected_virt_end = virt + size;
#endif

    if (size == 0)
        return true;

    mmu_granularity g = h->cfg_.lo_gran;
    mmu_tbl tbl0 = tbl0_from_handle(h);
    mmu_tbl tbl = tbl0;

    mmu_tbl_level l, target_lvl;
    size_t cover;
    size_t i;

    if (size % g != 0 || virt % g != 0)
        return false;


    while (size > 0) {
        tbl = tbl0;

        get_target_lvl(&target_lvl, &cover, size, g, virt, 0);


        DEBUG_ASSERT(size % g == 0 || virt % g == 0);
        DEBUG_ASSERT(virt % cover == 0);

        // the main loop of the fn, finds the actual table of the target level

        bool already_unmapped = false;

        for (l = MMU_TBL_LV0; l < target_lvl; l++) {
            i = table_index(virt, g, l);

            mmu_hw_pd pd = mmu_tbl_get_pd(tbl, i);

            // not valid
            if (!pd_get_valid(pd)) {
                already_unmapped = true;
                break;
            }

            switch (pd_get_type(pd)) {
                case MMU_PD_BLOCK:
                    tbl = split_block(h, tbl, i, l, info);
                    continue;
                case MMU_PD_TABLE:
                    tbl = tbl_from_td(pd, g);
                    continue;
                default:
                    PANIC("mmu_map: err");
            }
        }

        if (already_unmapped) {
            cover = min(size, pd_cover_bytes(g, l));
            size -= cover;
            virt += cover;
            continue;
        }

        // build the null block descriptor
        i = table_index(virt, g, target_lvl);
        mmu_hw_pd old = mmu_tbl_get_pd(tbl, i);

        tbl.pds[i] = NULL_PD;

        // if it was a table, free it (and all the subtables)
        if (pd_get_type(old) == MMU_PD_TABLE && pd_get_valid(old))
            UNLOCKED_free_subtree(h, tbl_from_td(old, g), info);

        size -= cover;
        virt += cover;
    }


    // TODO: Collapse null tables
    /*
        tbl = tbl0;
        // another final look to see if a table is fully null and collapse it
        for (l = MMU_TBL_LV0; l <= max_level(g); l++) {
            i = table_index(virt, g, l);

            mmu_hw_pd pd = tbl.pds[i];

            if (!pd_get_valid(pd))
                break;

            if (pd_get_type(pd) == MMU_PD_TABLE) {
                mmu_tbl subtbl = tbl_from_td(pd, g);

                if (tbl_is_null(subtbl, g)) {
                    tbl.pds[i] = NULL_PD;

                    UNLOCKED_free_subtree(h, tbl_from_td(pd, g), info);
                    break;
                }

                tbl = subtbl;
            }
        }
    */

#ifdef DEBUG
    DEBUG_ASSERT(size == 0 && virt == expected_virt_end);
#endif

    return true;
}


bool mmu_unmap(mmu_handle* h, v_uintptr virt, size_t size, mmu_op_info* info)
{
    bool result;

    if (info)
        *info = (mmu_op_info) {
#ifdef DEBUG
            .iters = 0,
#endif
            .alocated_tbls = 0,
            .freed_tbls = 0,
        };

    spin_lock(&h->slock_);
    result = UNLOCKED_unmap_(h, virt, size, info);
    spin_unlock(&h->slock_);

    return result;
}


static inline bool mmu_supports_4kb_(uint64 id_aa64mmfr0)
{
    return (((id_aa64mmfr0 >> 28) & 0xF) == 0);
}

//  DDI0500J_cortex_a53_trm.pdf p.104
static inline bool mmu_supports_64kb_(uint64 id_aa64mmfr0)
{
    return (((id_aa64mmfr0 >> 24) & 0xF) == 0);
}

//  DDI0500J_cortex_a53_trm.pdf p.104
static inline bool mmu_supports_16kb_(uint64 id_aa64mmfr0)
{
    return (((id_aa64mmfr0 >> 20) & 0xF) == 0);
}

static void mmu_apply(mmu_handle* h)
{
    mmu_granularity g[2] = {h->cfg_.lo_gran, h->cfg_.hi_gran};
    mmu_tbl tbl0 = tbl0_from_handle(h);
    mmu_tbl tbl1 = tbl1_from_handle(h);


    ASSERT(tbl0.pds && tbl1.pds);


    irq_spinlocked(&h->slock_)
    {
        uint64 id_aa64mmfr0 = _mmu_get_ID_AA64MMFR0_EL1();

        for (size_t i = 0; i < 2; i++)
            switch (g[i]) {
                case MMU_GRANULARITY_4KB:
                    if (!mmu_supports_4kb_(id_aa64mmfr0))
                        PANIC("4KB granularity not supported");
                    break;
                case MMU_GRANULARITY_16KB:
                    if (!mmu_supports_16kb_(id_aa64mmfr0))
                        PANIC("16KB granularity not supported");
                    break;
                case MMU_GRANULARITY_64KB:
                    if (!mmu_supports_64kb_(id_aa64mmfr0))
                        PANIC("64KB granularity not supported");
                    break;
                default:
                    PANIC("Unknown MMU granularity");
            }

        _mmu_set_TTBR0_EL1((uintptr)tbl0.pds);
        _mmu_set_TTBR1_EL1((uintptr)tbl1.pds);
        mmu_apply_cfg(h->cfg_);
    }
}
