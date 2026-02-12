#include "reserve_malloc.h"

#include <kernel/panic.h>
#include <lib/mem.h>
#include <lib/stdbitfield.h>

#include "early_kalloc.h"
#include "kernel/mm.h"
#include "lib/stdbool.h"


#define RESERVE_MALLOC_SIZE BITFIELD_CAPACITY(bitfield32)
const size_t RESERVE_MALLOC_RESERVE_SIZE = RESERVE_MALLOC_SIZE;


static pv_ptr reserved_addr[RESERVE_MALLOC_SIZE];
static bitfield32 reserved_pages;


void reserve_malloc_init()
{
    ASSERT(RESERVE_MALLOC_SIZE <= bitfield_bit_size(reserved_pages));

    reserved_pages = 0;

    for (size_t i = 0; i < RESERVE_MALLOC_SIZE; i++) {
        ASSERT(!bitfield_get(reserved_pages, i));

        p_uintptr pa = early_kalloc(KPAGE_SIZE, "reserve_allocator_page", false, false);
        v_uintptr va = mm_kpa_to_kva(pa); // works because all the memblocks are assured to be
                                          // mapped with the kernel physmap offset


        pv_ptr pv = pv_ptr_new(pa, va);


        ASSERT(pv.pa != 0 && ptrs_are_kmapped(pv));
        reserved_addr[i] = pv;

        bitfield_set_high(reserved_pages, i);
    }


    ASSERT(reserved_pages == (typeof(reserved_pages))((1ULL << RESERVE_MALLOC_SIZE) - 1));
}


pv_ptr reserve_malloc()
{
    for (size_t i = 0; i < RESERVE_MALLOC_SIZE; i++) {
        if (bitfield_get(reserved_pages, i)) {
            pv_ptr pmap = reserved_addr[i];
            bitfield_clear(reserved_pages, i);

            DEBUG_ASSERT(ptrs_are_kmapped(pmap));

            return pmap;
        }
    }

    PANIC("reserve_malloc: no more reserved pages available");
}


void reserve_malloc_fill()
{
    if (reserved_pages == (typeof(reserved_pages))((1ULL << RESERVE_MALLOC_SIZE) - 1))
        return;

    for (size_t i = 0; i < RESERVE_MALLOC_SIZE; i++) {
        if (bitfield_get(reserved_pages, i))
            break;


        raw_kmalloc_cfg cfg = RAW_KMALLOC_DEFAULT_CFG;
        cfg.fill_reserve = false;
        cfg.kmap = true;
        cfg.assign_pa = true;


        v_uintptr va = (v_uintptr)raw_kmalloc(1, "reserve_malloc page", &cfg);
        p_uintptr pa = mm_kva_to_kpa(va);

        pv_ptr pv = pv_ptr_new(pa, va);


        DEBUG_ASSERT(pv.pa != 0 && ptrs_are_kmapped(pv));
        reserved_addr[i] = pv;

        bitfield_set_high(reserved_pages, i);
    }

    DEBUG_ASSERT(reserved_pages == (typeof(reserved_pages))((1ULL << RESERVE_MALLOC_SIZE) - 1));
}