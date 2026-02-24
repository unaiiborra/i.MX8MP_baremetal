#include "arm/mmu/mmu.h"
#include "kernel/io/stdio.h"
#include "kernel/panic.h"
#include "lib/stdmacros.h"
#include "lib/string.h"
#include "mmu_dc.h"
#include "mmu_helpers.h"
#include "mmu_types.h"

// TESTING, llm assisted


static void dbg_puts(const char* s)
{
    kprint(s);
}

static void dbg_indent(size_t n)
{
    for (size_t i = 0; i < n; i++)
        dbg_puts("  ");
}


static void dbg_u64(uint64 v)
{
    char buf[200];
    stdint_to_ascii((STDINT_UNION) {.uint64 = v}, STDINT_UINT64, buf, sizeof(buf),
                    STDINT_BASE_REPR_HEX);
    dbg_puts(buf);
}

static void dbg_dec(uint64 v)
{
    char buf[32];
    stdint_to_ascii((STDINT_UNION) {.uint64 = v}, STDINT_UINT64, buf, sizeof(buf),
                    STDINT_BASE_REPR_DEC);
    dbg_puts(buf);
}


void mmu_debug_dump_tbl_(mmu_handle* h, mmu_tbl tbl, mmu_tbl_rng ttbrx, mmu_tbl_level lvl,
                         v_uintptr va_base, size_t indent)
{
    mmu_granularity g = ttbrx == MMU_TBL_LO ? h->cfg_.lo_gran : h->cfg_.hi_gran;
    size_t entries = tbl_entries(g);
    size_t cover = dc_cover_bytes(g, lvl);

    for (size_t i = 0; i < entries; i++) {
        mmu_hw_dc dc = tbl.dcs[i];

        if (!dc_get_valid(dc))
            continue;

        v_uintptr va = va_base + i * cover;

        dbg_indent(indent);

        dbg_puts("[L");
        dbg_dec(lvl);
        dbg_puts(" ");
        dbg_dec(i);
        dbg_puts("/");
        dbg_dec(entries - 1);
        dbg_puts("] VA ");
        dbg_u64(ttbrx == MMU_TBL_LO ? va : va | 0xFFFF000000000000);
        dbg_puts(" -> ");

        switch (dc_get_type(dc, g, lvl)) {
            case MMU_DESCRIPTOR_TABLE: {
                p_uintptr pa = dc_get_output_address(dc, g);

                dbg_puts("TABLE @ PA ");
                dbg_u64(pa);
                dbg_puts(" - dc[");
                dbg_u64(dc.v);
                dbg_puts("]");
                dbg_puts("\n\r");

                mmu_debug_dump_tbl_(h, tbl_from_td(h, dc, g, lvl), ttbrx, lvl + 1, va, indent + 1);
                break;
            }

            case MMU_DESCRIPTOR_BLOCK: {
                p_uintptr pa = dc_get_output_address(dc, g);

                dbg_puts("BLOCK PA ");
                dbg_u64(pa);
                dbg_puts(" size ");
                dbg_dec(cover);
                dbg_puts(" - dc[");
                dbg_u64(dc.v);
                dbg_puts("]");
                dbg_puts("\n\r");
                break;
            }

            case MMU_DESCRIPTOR_PAGE: {
                p_uintptr pa = dc_get_output_address(dc, g);

                dbg_puts("PAGE PA ");
                dbg_u64(pa);
                dbg_puts(" size ");
                dbg_dec(cover);
                dbg_puts(" - dc[");
                dbg_u64(dc.v);
                dbg_puts("]");
                dbg_puts("\n\r");
                break;
            }

            default:
                dbg_puts("UNKNOWN!\n\r");
                break;
        }
    }
}


void mmu_debug_dump(mmu_handle* h, mmu_tbl_rng ttbrx)
{
    mmu_debug_dump_tbl_(h, ttbrx == MMU_TBL_LO ? ttbr0_from_handle(h) : ttbr1_from_handle(h), ttbrx,
                        MMU_TBL_LV0, 0, 0);
}


void mmu_stress_test(mmu_handle* h, mmu_tbl_rng ttbrx, mmu_pg_cfg cfg, v_uintptr va_start,
                     v_uintptr va_end)
{
    mmu_op_info info;
    bool ok;

    kprint("\n\r=== MMU STRESS TEST BEGIN ===\n\r");

    /* ------------------------------------------------------------ */
    /* 1) Full 1:1 map of VA range                                  */
    /* ------------------------------------------------------------ */
    kprint("\n\r[TEST 1] Full 1:1 mapping of VA range\n\r");

    ok = mmu_map(h, va_start, 0, va_end - va_start, cfg, &info);
    ASSERT(ok);

    kprint("[TEST 1] Table dump\n\r");
    mmu_debug_dump(h, ttbrx);

    /* ------------------------------------------------------------ */
    /* 2) Unmap by halves (break large blocks)                      */
    /* ------------------------------------------------------------ */
    kprint("\n\r[TEST 2] Unmap by halves\n\r");

    ok = mmu_unmap(h, va_start, (va_end - va_start) / 2, &info);
    ASSERT(ok);

    ok = mmu_unmap(h, va_start + (va_end - va_start) / 2, (va_end - va_start) / 2, &info);
    ASSERT(ok);

    kprint("[TEST 2] Done\n\r");

    /* ------------------------------------------------------------ */
    /* 3) Incremental map with decreasing sizes                     */
    /* ------------------------------------------------------------ */
    kprint("\n\r[TEST 3] Incremental mapping with decreasing sizes\n\r");

    size_t sizes[] = {
        MEM_GiB * 1, MEM_MiB * 512, MEM_MiB * 128, MEM_MiB * 2, MEM_KiB * 64, MEM_KiB * 4,
    };

    v_uintptr va = va_start;
    p_uintptr pa = 0;

    for (size_t s = 0; s < ARRAY_LEN(sizes); s++) {
        kprint("  - Switching to smaller block size\n\r");

        while (va + sizes[s] <= va_end) {
            ok = mmu_map(h, va, pa, sizes[s], cfg, &info);
            ASSERT(ok);

            va += sizes[s];
            pa += sizes[s];
        }
    }

    kprint("[TEST 3] Table dump\n\r");
    mmu_debug_dump(h, ttbrx);

    /* ------------------------------------------------------------ */
    /* 4) Alternating unmap (interleaved holes)                     */
    /* ------------------------------------------------------------ */
    kprint("\n\r[TEST 4] Alternating unmap (interleaved holes)\n\r");

    for (v_uintptr v = va_start; v < va_end; v += MEM_MiB * 2) {
        ok = mmu_unmap(h, v, MEM_MiB * 1, &info);
        ASSERT(ok);
    }

    kprint("[TEST 4] Table dump\n\r");
    mmu_debug_dump(h, ttbrx);

    /* ------------------------------------------------------------ */
    /* 5) Remap holes with different PA                             */
    /* ------------------------------------------------------------ */
    kprint("\n\r[TEST 5] Remap holes with different PA\n\r");

    for (v_uintptr v = va_start; v < va_end; v += MEM_MiB * 2) {
        ok = mmu_map(h, v, v + MEM_GiB, MEM_MiB * 1, cfg, &info);
        ASSERT(ok);
    }

    kprint("[TEST 5] Table dump\n\r");
    mmu_debug_dump(h, ttbrx);

    /* ------------------------------------------------------------ */
    /* 6) Fine-grained stress: 4 KiB pages                          */
    /* ------------------------------------------------------------ */
    kprint("\n\r[TEST 6] Fine-grained stress: 4 KiB pages\n\r");

    for (v_uintptr v = va_start; v < va_end; v += MEM_KiB * 4) {
        ok = mmu_unmap(h, v, MEM_KiB * 4, &info);
        ASSERT(ok);

        ok = mmu_map(h, v, v, MEM_KiB * 4, cfg, &info);
        ASSERT(ok);
    }

    kprint("[TEST 6] Table dump\n\r");
    mmu_debug_dump(h, ttbrx);

    /* ------------------------------------------------------------ */
    /* 7) Full cleanup                                              */
    /* ------------------------------------------------------------ */
    kprint("\n\r[TEST 7] Full cleanup (unmap entire VA range)\n\r");

    ok = mmu_unmap(h, va_start, va_end - va_start, &info);
    ASSERT(ok);

    kprint("[TEST 7] Final table dump\n\r");
    mmu_debug_dump(h, ttbrx);

    kprint("\n\r=== MMU STRESS TEST END ===\n\r");
}
