#pragma once


#include <kernel/panic.h>
#include <lib/stdbool.h>
#include <lib/stdint.h>

#include "lib/math.h"


static inline uintptr align_up(uintptr x, size_t a)
{
	DEBUG_ASSERT(is_pow2(a), "can only align powers of 2");
	return (x + a - 1) & ~(a - 1);
}

static inline uintptr align_down(uintptr x, size_t a)
{
	DEBUG_ASSERT(is_pow2(a), "can only align powers of 2");
	return x & ~(a - 1);
}

#define align_up_ptr(x, a)      ((void *)align_up((uintptr)(x), a))
#define align_down_ptr(x, a)    ((void *)align_down((uintptr)(x), a))


static inline bool is_aligned(uintptr x, size_t a)
{
	DEBUG_ASSERT(is_pow2(a), "can only align powers of 2");
	return (x & (a - 1)) == 0;
}


// https://en.wikipedia.org/wiki/Offsetof
#define offsetof(st, m)    __builtin_offsetof(st, m)
