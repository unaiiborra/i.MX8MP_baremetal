/*
    The early allocator used by all the other allocators like the page allocator to allocate itself,
   and also the physical pages struct. When the early phase of initializing the allocators has
   finished, the allocated pages information is passed to the other allocators to set the initial
   state of the memory layout of the kernel.
*/

#include "early_kalloc.h"

#include <arm/mmu/mmu.h>
#include <boot/panic.h>
#include <frdm_imx8mp.h>
#include <lib/lock/spinlock.h>
#include <lib/stdint.h>

#include "../mm_info.h"

#define MEMBLOCK_SIZE MMU_GRANULARITY_4KB


_Alignas(16) static spinlock_t early_kallock_lock_;


_Alignas(16) static size_t memblock_struct_count_;
_Alignas(16) static memblock* next_memblock_;
_Alignas(16) static memblock* memblocks_start_;

_Alignas(16) static uintptr current_phys_;

_Alignas(16) static bool early_kalloc_stage_ended_;


void early_kalloc_init()
{
    early_kalloc_stage_ended_ = false;
    memblock_struct_count_ = 0;


    memblocks_start_ = (memblock*)(mm_info_ddr_end() - sizeof(memblock));
    next_memblock_ = memblocks_start_;

    current_phys_ = 0;

    spinlock_init(&early_kallock_lock_);
}


static void UNLOCKED_early_kalloc_(void** addr, size_t bytes, const char* tag, bool permanent)
{
    size_t blocks = (bytes + MEMBLOCK_SIZE - 1) / MEMBLOCK_SIZE;


    ASSERT(!early_kalloc_stage_ended_, "early memory initialization stage has already ended, "
                                       "early_kalloc is not usable anymore");

    p_uintptr next_phys = current_phys_ + blocks * MEMBLOCK_SIZE;
    p_uintptr next_meta = (p_uintptr)(next_memblock_ - 1);

    ASSERT(next_phys <= next_meta, "early_kalloc has no more physical memory");


    *next_memblock_ = (memblock) {
        .addr = current_phys_,
        .blocks = blocks,
        .tag = tag,
        .permanent = permanent,
    };

    
    *addr = (void*)next_memblock_->addr;

    memblock_struct_count_++;
    current_phys_ += blocks * MEMBLOCK_SIZE;
    next_memblock_--; // allocates its own memory structure downwards from ram end

    DEBUG_ASSERT(current_phys_ % MEMBLOCK_SIZE == 0);
}


p_uintptr early_kalloc(size_t bytes, const char* tag, bool permanent)
{
    void* addr;

    // should not be needed as multithreading for this stage is not expected, but just in case I
    // decide to use multithreading for initializing
    spinlocked(&early_kallock_lock_)
    {
        UNLOCKED_early_kalloc_(&addr, bytes, tag, permanent);
    }

    return (p_uintptr)addr;
}


static void UNLOCKED_finish_early_kalloc_stage_()
{
    // make size for the memblocks structs themselves.
    size_t meta_bytes = sizeof(memblock) * (memblock_struct_count_ + 1);

    void* addr;
    UNLOCKED_early_kalloc_(&addr, meta_bytes, "early_kalloc", false);

    // last backwards placed memblock (placed inverse (from bigger addresses to smaller))
    memblock* last_memblock = memblocks_start_ - memblock_struct_count_;
    // address of where the memblocks are going to be copied forwards
    memblock* first_memblock_cpy = (memblock*)addr;

    for (size_t i = 0; i < memblock_struct_count_; i++)
        first_memblock_cpy[i] = last_memblock[memblock_struct_count_ - i];

    // memblocks_start now represents the forward array of memblock information
    memblocks_start_ = first_memblock_cpy;
}


void early_kalloc_get_memblocks(memblock** memblocks, size_t* memblock_count)
{
    spinlocked(&early_kallock_lock_)
    {
        bool first_call = !early_kalloc_stage_ended_;

        if (first_call)
            UNLOCKED_finish_early_kalloc_stage_();

        *memblocks = memblocks_start_;

        early_kalloc_stage_ended_ = true;

        *memblock_count = memblock_struct_count_;
    }
}
