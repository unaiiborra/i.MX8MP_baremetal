#include "reloc.h"

#include <lib/mem.h>
#include <lib/stdbool.h>
#include <lib/stdmacros.h>

#include "../malloc/early_kalloc.h"
#include "../mm_info.h"
#include "arm/mmu/mmu.h"
#include "kernel/mm.h"
#include "kernel/panic.h"
#include "lib/unit/mem.h"

extern _Noreturn void _jmp_to_with_offset(void* to, size_t offset);
extern _Noreturn void _reloc_cfg_end(void);


void mm_reloc_kernel()
{
    ASSERT(mmu_is_active());


    // jumps to the asm fn _reloc_cfg_end (relocated with KERNEL BASE). _reloc_cfg_end returns to
    // reloc_cfg_end
    _jmp_to_with_offset(_reloc_cfg_end, KERNEL_BASE);
}


void reloc_cfg_end()
{
    // reloc the allocated tables pointers offsets
    mmu_reloc(&mm_mmu_h, KERNEL_BASE);


    // get first free heap va
    memblock* mblcks;
    size_t n;
    early_kalloc_get_memblocks(&mblcks, &n);
    v_uintptr free_heap_start =
        mm_kpa_to_kva(mblcks[n - 1].addr + (mblcks[n - 1].blocks * KPAGE_SIZE));


    // unmap the identity mapping
    mmu_unmap(&mm_mmu_h, 0x0, MEM_GiB * 5, NULL);
    mmu_unmap(&mm_mmu_h, free_heap_start, MEM_TiB, NULL);


    extern _Noreturn void _return_to_kernel_entry(size_t physmap_offset);
    _return_to_kernel_entry(KERNEL_BASE);
}
