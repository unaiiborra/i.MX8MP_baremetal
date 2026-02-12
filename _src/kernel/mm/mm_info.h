#pragma once

#include <arm/mmu/mmu.h>
#include <lib/stdint.h>

#include "lib/mem.h"


typedef struct {
    v_uintptr start;
    v_uintptr end;
    size_t size;
} mm_ksection;

typedef struct {
    mm_ksection text;
    mm_ksection rodata;
    mm_ksection data;
    mm_ksection bss;
    mm_ksection stacks;
    mm_ksection heap;
} mm_ksections;


void mm_info_init();


size_t mm_info_max_ddr_size(void);
size_t mm_info_ddr_size(void);

uintptr mm_info_ddr_start(void);
uintptr mm_info_ddr_end(void);

uintptr mm_info_kernel_start(void);

size_t mm_info_page_count(void);

size_t mm_info_mm_addr_space(void);


extern const mm_ksections MM_KSECTIONS;

/// the kernel mmu handle, not exposed in a global header as only the mm system should access it
extern mmu_handle mm_mmu_h;