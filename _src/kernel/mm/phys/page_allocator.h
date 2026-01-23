#pragma once
#include <lib/stdint.h>

#include "../init/early_kalloc.h"
#include "lib/mem.h"
#include "page.h"


typedef struct {
    size_t order;
    p_uintptr phys;
    mm_page_data data;
} mm_page;


#define NONE (~(size_t)0)
#define IS_NONE(v) ((v) == NONE)


void page_allocator_init();


mm_page page_malloc(size_t order, const char* tag);
void page_free(mm_page p);
