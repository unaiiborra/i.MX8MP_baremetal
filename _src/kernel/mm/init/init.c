#include <frdm_imx8mp.h>
#include <kernel/io/stdio.h>
#include <kernel/mm.h>
#include <kernel/panic.h>

#include "../init/mem_regions/early_kalloc.h"
#include "../malloc/cache_malloc/cache_malloc.h"
#include "../malloc/internal/reserve_malloc.h"
#include "../malloc/raw_kmalloc/raw_kmalloc.h"
#include "../mm_info.h"
#include "../mm_mmu/mm_mmu.h"
#include "../phys/page_allocator.h"
#include "../reloc/reloc.h"
#include "../virt/vmalloc.h"
#include "arm/mmu.h"
#include "identity_mapping.h"
#include "lib/stdmacros.h"

static p_uintptr mmu_allocator_fn(size_t bytes)
{
	DEBUG_ASSERT(bytes == KPAGE_SIZE);

	pv_ptr pv = reserve_malloc("mmu table");

	DEBUG_ASSERT(pv.pa % KPAGE_SIZE == 0);

	return pv.va;
}


// allocator freer for the early identity mapping tables
static void mmu_free_fn(p_uintptr addr)
{
	raw_kfree((void *)addr);
}


void mm_early_init()
{
	mm_info_init();

	// init early kalloc
	early_kalloc_init();

	// init MM_MMU_UNMAPPED_LO
	mm_mmu_early_init();

	// init identity mapping
	early_identity_mapping();

	// page allocator
	page_allocator_init();

	// virtual allocator
	vmalloc_init();

	// reserve allocator
	reserve_malloc_init();


	early_memreg *mregs;
	size_t n;
	early_kalloc_get_memregs(&mregs, &n);


	page_allocator_update_memregs(mregs, n);
	vmalloc_update_memregs(mregs, n);


	mmu_mapping * const EARLY_MMU_MAPPINGS[2] = {
		&early_lo_mapping,
		&kernel_mmu_mapping,
	};

	for (size_t i = 0; i < ARRAY_LEN(EARLY_MMU_MAPPINGS); i++) {
		mmu_mapping_set_allocator(
			EARLY_MMU_MAPPINGS[i],
			mm_as_kva_ptr((void *)mmu_allocator_fn));

		mmu_mapping_set_allocator_free(
			EARLY_MMU_MAPPINGS[i],
			mm_as_kva_ptr((void *)mmu_free_fn));

		mmu_mapping_set_physmap_offset(EARLY_MMU_MAPPINGS[i], KERNEL_BASE);
	}

	// reloc kernel: returns to the kernel_entry() with the kernel relocated and
	// the sp resetted
	mm_reloc_kernel();
}


void mm_init()
{
	raw_kmalloc_init();

	cache_malloc_init();
}
