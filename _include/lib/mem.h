#pragma once

#include <lib/stdbool.h>
#include <lib/stdint.h>

#include "kernel/panic.h"

// phys address uintptr
typedef uintptr p_uintptr;

// virt address uintptr
typedef uintptr v_uintptr;


typedef struct {
    p_uintptr pa;
    v_uintptr va;
} pv_ptr;

static inline pv_ptr pv_ptr_new(p_uintptr pa, v_uintptr va)
{
    return (pv_ptr) {pa, va};
}

#define uintptr_p_to_ptr(type, uintptr_phys) ((type*)(uintptr_phys))
#define PTR_TO_UINTPTR_P(ptr) ((p_uintptr)(ptr))

#define UINTPTR_V_TO_PTR(type, uintptr_virt) ((type*)(uintptr_virt))
#define PTR_TO_UINTPTR_V(ptr) ((v_uintptr)(ptr))


size_t pa_supported_bits();


static inline bool address_is_valid(uintptr a, size_t bits, bool sign_extended)
{
    ASSERT(bits > 0 && bits <= 64);

    if (bits == 64)
        return true;

    uint64 upper_mask = ~((1ULL << bits) - 1);

    if (!sign_extended)
        return (upper_mask & a) == 0;

    uint64 sign_bit = 1ULL << (bits - 1);

    return (a & sign_bit) ? (a & upper_mask) == upper_mask : (a & upper_mask) == 0;
}

static inline v_uintptr va_sign_extend(v_uintptr va, size_t bits)
{
    if (bits == 64)
        return va;

    ASSERT(bits > 0 && bits < 64);
    ASSERT(address_is_valid(va, bits, false));

    uint64 sign_bit = 1ULL << (bits - 1);
    uint64 mask = ~((1ULL << bits) - 1);

    v_uintptr a = (va & sign_bit) ? (va | mask) : va;

    DEBUG_ASSERT(address_is_valid(a, bits, true));

    return a;
}

static inline v_uintptr va_zero_extend(v_uintptr va, size_t bits)
{
    if (bits == 64)
        return va;

    ASSERT(bits > 0 && bits < 64);
    ASSERT(address_is_valid(va, bits, true));

    v_uintptr a = va & ((1ULL << bits) - 1);

    DEBUG_ASSERT(address_is_valid(a, bits, false));

    return a;
}


/*
    Mem ctrl fns
*/

// TODO: maybe doing a c wrapper makes sense for checking if simd is enabled at
// lower EL levels
#define memcpy(dst, src, size) _memcpy(dst, src, size)

/// Standard memcpy, requieres simd instructions to be enabled
extern void* _memcpy(void* dst, const void* src, uint64 size);

/// Panics: if the size is not divisible by 64
void* memcpy64(void* dst, const void* src, uint64 size);

/// Panics: if the addreses are not aligned to 16 bytes or the size is not
/// divisible by 64
void* memcpy64_aligned(void* dst, const void* src, uint64 size);

#ifdef TEST
void test_memcpy(size_t size_start);
#endif
