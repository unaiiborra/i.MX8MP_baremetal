#include <frdm_imx8mp.h>
#include <kernel/panic.h>
#include <lib/mem.h>

#include "../malloc/early_kalloc.h"
#include "../malloc/raw_kmalloc.h"
#include "../malloc/reserve_malloc.h"
#include "../mm_info.h"
#include "../phys/page_allocator.h"
#include "../reloc/reloc.h"
#include "../virt/vmalloc.h"
#include "arm/mmu/mmu.h"
#include "identity_mapping.h"
#include "kernel/mm.h"
#include "lib/unit/mem.h"

static void early_reserve_device_and_kernel_mem()
{
    // TODO: not hardcoded

    /* reserve memory for mmio */
    early_kalloc(MEM_GiB * 1, "mmio", true, true);


    /* reserve memory for TF-A reserved zone */
    size_t tf_a_bytes = mm_info_kernel_start() - mm_info_ddr_start();
    ASSERT(tf_a_bytes % MMU_GRANULARITY_4KB == 0);
    early_kalloc(tf_a_bytes, "tf_a_protected", true, false);


    /* reserve memory for the text + bss + rodata + data + el2 + el1 stacks of the kernel */
    size_t kernel_fixed_size = MM_KSECTIONS.heap.start - MM_KSECTIONS.text.start;
    ASSERT(kernel_fixed_size % MMU_GRANULARITY_4KB == 0);
    early_kalloc(kernel_fixed_size, "kernel_fixed", true, false);
}


static p_uintptr mmu_allocator_fn(size_t bytes, size_t align)
{
    ASSERT(bytes == KPAGE_SIZE && align == KPAGE_SIZE);

    pv_ptr pv = reserve_malloc();

    return pv.va;
}


// allocator freer for the early identity mapping tables
static void mmu_free_fn(p_uintptr addr)
{
    (void)addr;

    PANIC("TODO: implement");
    // TODO: use the raw_allocator free
}


void mm_early_init()
{
    mm_info_init();

    // init early kalloc
    early_kalloc_init();
    early_reserve_device_and_kernel_mem();


    // init identity mapping
    early_identity_mapping();
#ifdef DEBUG
    term_prints("Identity mapping mmu: \n\r");
    mmu_debug_dump(&mm_mmu_h, MMU_TBL_LO);
    mmu_debug_dump(&mm_mmu_h, MMU_TBL_HI);
#endif

    // page allocator
    page_allocator_init();
#ifdef DEBUG
    term_prints("\n\rPage allocator test: \n\r");
    page_allocator_debug();
#endif

    // virtual allocator
    vmalloc_init();

    // reserve allocator
    reserve_malloc_init();


    memblock* mblcks;
    size_t n;
    early_kalloc_get_memblocks(&mblcks, &n);


    pv_ptr first_free_address = {
        .pa = page_allocator_update_memblocks(mblcks, n),
        .va = vmalloc_update_memblocks(mblcks, n),
    };

    ASSERT(ptrs_are_kmapped(first_free_address));


    mmu_reconfig_allocators(&mm_mmu_h, mm_kpa_to_kva_ptr(mmu_allocator_fn),
                            mm_kpa_to_kva_ptr(mmu_free_fn));


    // reloc kernel: returns to the kernel_entry() with the kernel relocated and the sp resetted
    mm_reloc_kernel();
}


void mm_init()
{
    raw_kmalloc_init();
}
