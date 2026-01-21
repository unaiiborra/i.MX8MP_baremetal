#pragma once
// https://github.com/bztsrc/raspi3-tutorial/blob/master/10_virtualmemory/mmu.c
// https://documentation-service.arm.com/static/63a43e333f28e5456434e18b?token=

#include <lib/lock/spinlock.h>
#include <lib/mem.h>
#include <lib/stdbool.h>
#include <lib/unit/mem.h>


typedef enum {
    MMU_GRANULARITY_4KB = 4 * MEM_KiB,
    MMU_GRANULARITY_16KB = 16 * MEM_KiB,
    MMU_GRANULARITY_64KB = 64 * MEM_KiB,
} mmu_granularity;


typedef void* (*mmu_alloc)(size_t bytes, size_t alignment);
typedef void (*mmu_free)(void* addr);


typedef struct {
    void* tbl0_;
    mmu_granularity g_;

    mmu_alloc alloc_;
    mmu_free free_;

    spinlock_t slock_;
} mmu_handle;


typedef enum {
    MMU_AP_EL0_NONE_EL1_RW = 0b00,
    MMU_AP_EL0_RW_EL1_RW = 0b01,
    MMU_AP_EL0_NONE_EL1_RO = 0b10,
    MMU_AP_EL0_RO_EL1_RO = 0b11,
} mmu_access_permission;

typedef struct {
    uint8 attr_index;
    mmu_access_permission ap;
    uint8 shareability;

    bool non_secure;
    bool access_flag;
    bool pxn;
    bool uxn;

    uint8 sw;
} mmu_cfg;


typedef struct {
#ifdef DEBUG
    size_t iters;
#endif
    size_t alocated_tbls;
    size_t freed_tbls;
} mmu_op_info;


// initializes the private structures needed for mmu control. Does not activate the mmu
void mmu_init(mmu_handle* h, mmu_granularity g, mmu_alloc alloc, mmu_free free);


mmu_cfg mmu_cfg_new(uint8 attr_index, mmu_access_permission ap, uint8 shareability, bool non_secure,
                    bool access_flag, bool pxn, bool uxn, uint8 sw);


void mmu_activate(mmu_handle h, bool d_cache, bool i_cache, bool align_trap);


bool mmu_map(mmu_handle h, v_uintptr virt, p_uintptr phys, size_t size, mmu_cfg cfg,
             mmu_op_info* info);

bool mmu_p_unmap(mmu_handle h, p_uintptr phys, size_t size, mmu_op_info* info);
bool mmu_unmap(mmu_handle h, v_uintptr virt, size_t size, mmu_op_info* info);


// debug
void mmu_debug_dump(mmu_handle h);

void mmu_stress_test(mmu_handle h, mmu_cfg cfg, v_uintptr va_start, v_uintptr va_end);