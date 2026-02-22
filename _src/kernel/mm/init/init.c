#include <frdm_imx8mp.h>
#include <kernel/io/term.h>
#include <kernel/mm.h>
#include <kernel/panic.h>

#include "../init/mem_regions/early_kalloc.h"
#include "../malloc/cache_malloc/cache_malloc.h"
#include "../malloc/internal/reserve_malloc.h"
#include "../malloc/raw_kmalloc/raw_kmalloc.h"
#include "../mm_info.h"
#include "../phys/page_allocator.h"
#include "../reloc/reloc.h"
#include "../virt/vmalloc.h"
#include "arm/mmu/mmu.h"
#include "identity_mapping.h"


static p_uintptr mmu_allocator_fn(size_t bytes, size_t align)
{
    ASSERT(bytes == KPAGE_SIZE && align == KPAGE_SIZE);

    pv_ptr pv = reserve_malloc("mmu table");

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


    early_memreg* mregs;
    size_t n;
    early_kalloc_get_memregs(&mregs, &n);


    page_allocator_update_memregs(mregs, n);
    vmalloc_update_memregs(mregs, n);


    mmu_reconfig_allocators(&mm_mmu_h, mm_as_kva_ptr((void*)mmu_allocator_fn),
                            mm_as_kva_ptr((void*)mmu_free_fn));


    // reloc kernel: returns to the kernel_entry() with the kernel relocated and the sp resetted
    mm_reloc_kernel();
}


void mm_init()
{
    raw_kmalloc_init();

    cache_malloc_init();
}
