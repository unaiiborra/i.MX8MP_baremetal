#include "mm_info.h"

#include <frdm_imx8mp.h>
#include <lib/mem.h>
#include <lib/stdint.h>

#include "kernel/mm.h"
#include "kernel/panic.h"
#include "lib/math.h"


extern v_uintptr __text_start[];
extern v_uintptr __text_end[];
extern size_t __text_size[];

extern v_uintptr __rodata_start[];
extern v_uintptr __rodata_end[];
extern size_t __rodata_size[];

extern v_uintptr __data_start[];
extern v_uintptr __data_end[];
extern size_t __data_size[];

extern v_uintptr __bss_start[];
extern v_uintptr __bss_end[];
extern size_t __bss_size[];

extern v_uintptr __stacks_start[];
extern v_uintptr __stacks_end[];
extern size_t __stacks_size[];

extern v_uintptr __heap_start[];
extern v_uintptr __heap_end[];
extern size_t __heap_size[];


#define BUILD_KSECTION(section)                                                       \
    (mm_ksection)                                                                     \
    {                                                                                 \
        .start = (v_uintptr)__##section##_start, .end = (v_uintptr)__##section##_end, \
        .size = (size_t)__##section##_size,                                           \
    }

const mm_ksections MM_KSECTIONS = (mm_ksections) {
    .text = BUILD_KSECTION(text),
    .rodata = BUILD_KSECTION(rodata),
    .data = BUILD_KSECTION(data),
    .bss = BUILD_KSECTION(bss),
    .stacks = BUILD_KSECTION(stacks),
    .heap = BUILD_KSECTION(heap),
};


static size_t mm_max_ddr_size_;
static size_t mm_ddr_size_;

static p_uintptr mm_ddr_start_;
static p_uintptr mm_ddr_end_;

static p_uintptr mm_kernel_start_;

static size_t mm_page_count_;

static size_t mm_addr_space_;


// TODO: get it from other non hardcoded source
#define DDR_SIZE MEM_GiB * 4
#define DDR_START 0x40000000UL

/// all this definitions are mostly hardcoded for the imx8mp, the idea is that they should come from
/// a dtb file or any other valid source that allows the kernel to dynamically initialize itself
void mm_info_init()
{
    extern void _start();


    mm_max_ddr_size_ = DDR_SIZE;
    mm_ddr_size_ = FRDM_IMX8MP_MEM_SIZE;

    mm_ddr_start_ = DDR_START;
    mm_ddr_end_ = mm_ddr_start_ + mm_ddr_size_;

    mm_kernel_start_ = (p_uintptr)_start;


    mm_addr_space_ = mm_ddr_end_;


    mm_page_count_ = div_ceil(mm_addr_space_, KPAGE_SIZE);


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


size_t mm_info_page_count(void)
{
    return mm_page_count_;
}

size_t mm_info_mm_addr_space(void)
{
    return mm_addr_space_;
}