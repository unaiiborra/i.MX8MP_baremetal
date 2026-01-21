#include "arm/mmu/mmu.h"
#include "drivers/uart/uart.h"
#include "kernel/devices/drivers.h"
#include "lib/stdmacros.h"
#include "lib/string.h"
#include "mmu_helpers.h"
#include "mmu_pd.h"
#include "mmu_types.h"

// TESTING, llm assisted


static void dbg_puts(const char* s)
{
    UART_puts_sync(&UART2_DRIVER, s);
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


void mmu_debug_dump_tbl_(mmu_handle h, mmu_tbl tbl, mmu_tbl_level lvl, v_uintptr va_base,
                         size_t indent)
{
    mmu_granularity g = h.g_;
    size_t entries = tbl_entries(g);
    size_t cover = pd_cover_bytes(g, lvl);

    for (size_t i = 0; i < entries; i++) {
        mmu_hw_pd pd = tbl.pds[i];

        if (!pd_get_valid(pd))
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
        dbg_u64(va);
        dbg_puts(" -> ");

        switch (pd_get_type(pd)) {
            case MMU_PD_TABLE: {
                p_uintptr pa = pd_get_output_address(pd, g);

                dbg_puts("TABLE @ PA ");
                dbg_u64(pa);
                dbg_puts("\n\r");

                mmu_debug_dump_tbl_(h, tbl_from_td(pd, g), lvl + 1, va, indent + 1);
                break;
            }

            case MMU_PD_BLOCK: {
                p_uintptr pa = pd_get_output_address(pd, g);

                dbg_puts("BLOCK PA ");
                dbg_u64(pa);
                dbg_puts(" size ");
                dbg_dec(cover);
                dbg_puts("\n\r");
                break;
            }

            default:
                dbg_puts("UNKNOWN\n\r");
                break;
        }
    }
}


void mmu_debug_dump(mmu_handle h)
{
    mmu_debug_dump_tbl_(h, tbl0_from_handle(h), MMU_TBL_LV0, 0, 0);
}


void mmu_stress_test(mmu_handle h, mmu_cfg cfg, v_uintptr va_start, v_uintptr va_end)
{
    mmu_op_info info;
    bool ok;

    UART_puts_sync(&UART2_DRIVER, "\n\r=== MMU STRESS TEST BEGIN ===\n\r");

    /* ------------------------------------------------------------ */
    /* 1) Full 1:1 map of VA range                                  */
    /* ------------------------------------------------------------ */
    UART_puts_sync(&UART2_DRIVER, "\n\r[TEST 1] Full 1:1 mapping of VA range\n\r");

    ok = mmu_map(h, va_start, 0, va_end - va_start, cfg, &info);
    ASSERT(ok);

    UART_puts_sync(&UART2_DRIVER, "[TEST 1] Table dump\n\r");
    mmu_debug_dump(h);

    /* ------------------------------------------------------------ */
    /* 2) Unmap by halves (break large blocks)                      */
    /* ------------------------------------------------------------ */
    UART_puts_sync(&UART2_DRIVER, "\n\r[TEST 2] Unmap by halves\n\r");

    ok = mmu_unmap(h, va_start, (va_end - va_start) / 2, &info);
    ASSERT(ok);

    ok = mmu_unmap(h, va_start + (va_end - va_start) / 2, (va_end - va_start) / 2, &info);
    ASSERT(ok);

    UART_puts_sync(&UART2_DRIVER, "[TEST 2] Done\n\r");

    /* ------------------------------------------------------------ */
    /* 3) Incremental map with decreasing sizes                     */
    /* ------------------------------------------------------------ */
    UART_puts_sync(&UART2_DRIVER, "\n\r[TEST 3] Incremental mapping with decreasing sizes\n\r");

    size_t sizes[] = {
        MEM_GiB * 1, MEM_MiB * 512, MEM_MiB * 128, MEM_MiB * 2, MEM_KiB * 64, MEM_KiB * 4,
    };

    v_uintptr va = va_start;
    p_uintptr pa = 0;

    for (size_t s = 0; s < ARRAY_LEN(sizes); s++) {
        UART_puts_sync(&UART2_DRIVER, "  - Switching to smaller block size\n\r");

        while (va + sizes[s] <= va_end) {
            ok = mmu_map(h, va, pa, sizes[s], cfg, &info);
            ASSERT(ok);

            va += sizes[s];
            pa += sizes[s];
        }
    }

    UART_puts_sync(&UART2_DRIVER, "[TEST 3] Table dump\n\r");
    mmu_debug_dump(h);

    /* ------------------------------------------------------------ */
    /* 4) Alternating unmap (interleaved holes)                     */
    /* ------------------------------------------------------------ */
    UART_puts_sync(&UART2_DRIVER, "\n\r[TEST 4] Alternating unmap (interleaved holes)\n\r");

    for (v_uintptr v = va_start; v < va_end; v += MEM_MiB * 2) {
        ok = mmu_unmap(h, v, MEM_MiB * 1, &info);
        ASSERT(ok);
    }

    UART_puts_sync(&UART2_DRIVER, "[TEST 4] Table dump\n\r");
    mmu_debug_dump(h);

    /* ------------------------------------------------------------ */
    /* 5) Remap holes with different PA                             */
    /* ------------------------------------------------------------ */
    UART_puts_sync(&UART2_DRIVER, "\n\r[TEST 5] Remap holes with different PA\n\r");

    for (v_uintptr v = va_start; v < va_end; v += MEM_MiB * 2) {
        ok = mmu_map(h, v, v + MEM_GiB, MEM_MiB * 1, cfg, &info);
        ASSERT(ok);
    }

    UART_puts_sync(&UART2_DRIVER, "[TEST 5] Table dump\n\r");
    mmu_debug_dump(h);

    /* ------------------------------------------------------------ */
    /* 6) Fine-grained stress: 4 KiB pages                          */
    /* ------------------------------------------------------------ */
    UART_puts_sync(&UART2_DRIVER, "\n\r[TEST 6] Fine-grained stress: 4 KiB pages\n\r");

    for (v_uintptr v = va_start; v < va_end; v += MEM_KiB * 4) {
        ok = mmu_unmap(h, v, MEM_KiB * 4, &info);
        ASSERT(ok);

        ok = mmu_map(h, v, v, MEM_KiB * 4, cfg, &info);
        ASSERT(ok);
    }

    UART_puts_sync(&UART2_DRIVER, "[TEST 6] Table dump\n\r");
    mmu_debug_dump(h);

    /* ------------------------------------------------------------ */
    /* 7) Full cleanup                                              */
    /* ------------------------------------------------------------ */
    UART_puts_sync(&UART2_DRIVER, "\n\r[TEST 7] Full cleanup (unmap entire VA range)\n\r");

    ok = mmu_unmap(h, va_start, va_end - va_start, &info);
    ASSERT(ok);

    UART_puts_sync(&UART2_DRIVER, "[TEST 7] Final table dump\n\r");
    mmu_debug_dump(h);

    UART_puts_sync(&UART2_DRIVER, "\n\r=== MMU STRESS TEST END ===\n\r");
}
