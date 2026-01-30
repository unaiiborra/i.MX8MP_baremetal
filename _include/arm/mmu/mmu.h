#pragma once
// https://github.com/bztsrc/raspi3-tutorial/blob/master/10_virtualmemory/mmu.c
// https://documentation-service.arm.com/static/63a43e333f28e5456434e18b?token=

#include <lib/lock/spinlock.h>
#include <lib/mem.h>
#include <lib/stdbool.h>
#include <lib/stdint.h>
#include <lib/unit/mem.h>


typedef enum {
    MMU_GRANULARITY_4KB = 4 * MEM_KiB,
    MMU_GRANULARITY_16KB = 16 * MEM_KiB,
    MMU_GRANULARITY_64KB = 64 * MEM_KiB,
} mmu_granularity;


typedef enum {
    MMU_TBL_HI = 0,
    MMU_TBL_LO = 1,
} mmu_tbl_rng;


typedef void* (*mmu_alloc)(size_t bytes, size_t alignment);
typedef void (*mmu_free)(void* addr);


typedef struct {
    _Alignas(16) bool enable;
    _Alignas(16) bool d_cache;
    _Alignas(16) bool i_cache;
    _Alignas(16) bool align_trap;


    _Alignas(16) bool hi_enable;
    _Alignas(16) bool lo_enable;

    // the supported number of bits by the high and low translation tables. For 4k, the standard is
    // usually 48 bits (48 max, 39 min). other values than 4kb granularity and 48 bit addresses have
    // not been tested
    _Alignas(16) uint8 hi_va_addr_bits;
    _Alignas(16) uint8 lo_va_addr_bits;

    mmu_granularity hi_gran;
    mmu_granularity lo_gran;
} mmu_cfg;


typedef struct {
    void* tbl0_;
    void* tbl1_;

    mmu_alloc alloc_;
    mmu_free free_;

    spinlock_t slock_;
    mmu_cfg cfg_;
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
} mmu_pg_cfg;


typedef struct {
#ifdef DEBUG
    size_t iters;
#endif
    size_t alocated_tbls;
    size_t freed_tbls;
} mmu_op_info;


// initializes the private structures needed for mmu control.
void mmu_init(mmu_handle* h, mmu_cfg cfg, mmu_alloc alloc, mmu_free free);


mmu_pg_cfg mmu_pg_cfg_new(uint8 attr_index, mmu_access_permission ap, uint8 shareability,
                          bool non_secure, bool access_flag, bool pxn, bool uxn, uint8 sw);


mmu_cfg mmu_get_cfg(mmu_handle* h);
void mmu_reconfig(mmu_handle* h, mmu_cfg cfg, mmu_alloc alloc, mmu_free free);


bool mmu_map(mmu_handle* h, v_uintptr virt, p_uintptr phys, size_t size, mmu_pg_cfg cfg,
             mmu_op_info* info);

// bool mmu_p_unmap(mmu_handle h, p_uintptr phys, size_t size, mmu_op_info* info);
bool mmu_unmap(mmu_handle* h, v_uintptr virt, size_t size, mmu_op_info* info);


// debug
void mmu_debug_dump(mmu_handle* h, mmu_tbl_rng ttbrx);

void mmu_stress_test(mmu_handle* h, mmu_tbl_rng ttbrx, mmu_pg_cfg cfg, v_uintptr va_start,
                     v_uintptr va_end);