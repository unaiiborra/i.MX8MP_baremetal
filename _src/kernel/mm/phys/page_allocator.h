#pragma once

#include <lib/mem.h>
#include <lib/stdint.h>

#include "../init/mem_regions/early_kalloc.h"
#include "page.h"


typedef uint64 page_mdt_bf;

typedef struct {
    p_uintptr pa;
    mm_page_data data;
} mm_page;


void page_allocator_init();

/// allocates the early stage memory regions.
/// checking
void page_allocator_update_memregs(const early_memreg* mregs, size_t n);

p_uintptr page_malloc(uint8 order, mm_page_data p);
void page_free(p_uintptr pa);

const char* page_allocator_update_tag(p_uintptr pa, const char* new_tag);

// copies the data from the page to the provided address
bool page_allocator_get_data(p_uintptr pa, mm_page_data* data);
bool page_allocator_set_data(p_uintptr pa, mm_page_data data);


void page_allocator_debug();