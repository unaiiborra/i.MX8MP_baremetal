#include "kernel/mm.h"

#include <arm/mmu/mmu.h>
#include <frdm_imx8mp.h>
#include <kernel/panic.h>
#include <lib/stdint.h>

#include "lib/mem.h"


extern uintptr _get_pc(void);


mmu_handle mm_mmu_h;


bool mm_kernel_is_relocated()
{
    uintptr pc = _get_pc();

    return ((pc >> 47) & 0x1FFFFUL) == 0x1FFFFUL;
}




p_uintptr mm_kva_to_kpa(v_uintptr va)
{
    extern p_uintptr _mm_kva_to_kpa(v_uintptr pa);

    ASSERT(va >= KERNEL_BASE);

    return _mm_kva_to_kpa(va);
}


v_uintptr mm_kpa_to_kva(p_uintptr pa)
{
    extern v_uintptr _mm_kpa_to_kva(p_uintptr pa);

    ASSERT(pa < KERNEL_BASE);

    return _mm_kpa_to_kva(pa);
}


void mm_dbg_print_mmu()
{
    mmu_debug_dump(&mm_mmu_h, MMU_TBL_LO);
    mmu_debug_dump(&mm_mmu_h, MMU_TBL_HI);
}
