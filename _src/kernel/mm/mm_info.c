#include "mm_info.h"

#include <frdm_imx8mp.h>
#include <lib/mem.h>

#include "boot/panic.h"

const size_t MM_PAGE_BYTES = MMU_GRANULARITY_4KB;

static size_t mm_max_ddr_size_;
static size_t mm_ddr_size_;

static p_uintptr mm_ddr_start_;
static p_uintptr mm_ddr_end_;

static p_uintptr mm_kernel_start_;
static size_t mm_kernel_size_;

static size_t mm_page_count_;


void mm_info_init()
{
    extern p_uintptr __ddr_size;
    extern p_uintptr __ddr_start;
    extern p_uintptr
        __kernel_mem_start[]; // mem start, not the kernel itself (free memory for allocations etc)

    extern void _start();


    mm_max_ddr_size_ = __ddr_size;
    mm_ddr_size_ = FRDM_IMX8MP_MEM_SIZE;

    mm_ddr_start_ = __ddr_start;
    mm_ddr_end_ = mm_ddr_start_ + mm_ddr_size_;

    mm_kernel_start_ = (p_uintptr)_start;
    mm_kernel_size_ = (p_uintptr)__kernel_mem_start - mm_kernel_start_;

    // TODO: watch if it needs to add the tfa pages and the mmio as pages
    mm_page_count_ = (mm_ddr_end_ - mm_kernel_start_) / MM_PAGE_BYTES;


    ASSERT(mm_ddr_size_ <= mm_max_ddr_size_);
}

size_t mm_info_max_ddr_size(void)
{
    return mm_max_ddr_size_;
}

size_t mm_info_ddr_size(void)
{
    return mm_ddr_size_;
}

p_uintptr mm_info_ddr_start(void)
{
    return mm_ddr_start_;
}

p_uintptr mm_info_ddr_end(void)
{
    return mm_ddr_end_;
}

p_uintptr mm_info_kernel_start(void)
{
    return mm_kernel_start_;
}


size_t mm_info_kernel_size(void)
{
    return mm_kernel_size_;
}

size_t mm_info_page_count(void)
{
    return mm_page_count_;
}
