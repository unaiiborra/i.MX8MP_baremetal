#pragma once
#include <lib/stdint.h>

#include "../init/early_kalloc.h"
#include "lib/mem.h"
#include "page.h"


typedef struct {
    uint8 order;
    p_uintptr phys;
    mm_page_data data;
} mm_page;


static inline bool page_is_valid(mm_page p)
{
    return p.order != UINT8_MAX;
}


void page_allocator_init();
void page_allocator_reserve_memblocks(memblock* mblcks, size_t n);


#ifdef DEBUG
p_uintptr page_allocator_testing_init();

#endif


mm_page page_malloc(size_t order, mm_page_data p);

void page_free(mm_page p);

/// should not be prioritized over page_free(), the main purpose is to clean non permanent early
/// stage allocations
void page_free_by_tag(const char* tag);


void page_allocator_debug_pages(bool full_print);
void page_allocator_debug();
void page_allocator_validate();