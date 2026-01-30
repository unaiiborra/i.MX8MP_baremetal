#pragma once

#include <lib/stdbool.h>
#include <lib/stdint.h>


static inline uint32 log2_floor_u32(uint32 x)
{
    uint32 r = 0;
    while (x >>= 1)
        r++;
    return r;
}


static inline uint64 log2_floor_u64(uint64 x)
{
    uint64 r = 0;
    while (x >>= 1)
        r++;
    return r;
}

static inline uint64 power_of2(uint64 x)
{
    return 1ULL << x;
}


static inline uint64 square(uint64 x)
{
    return x * x;
}


static inline uint64 max(uint64 a, uint64 b)
{
    return a > b ? a : b;
}


static inline uint64 min(uint64 a, uint64 b)
{
    return a < b ? a : b;
}


static inline uint64 div_round_up(uint64 a, uint64 b)
{
    return (a + b - 1) / b;
}


static inline bool is_pow2(uint64 x)
{
    return (x & (x - 1)) == 0;
}


// https://jameshfisher.com/2018/03/30/round-up-power-2/
static inline uint32 next_pow2_u32(uint32 x)
{
    return x <= 1 ? 1u : 1u << (32 - __builtin_clz(x - 1));
}


static inline uint64 next_pow2_u64(uint64 x)
{
    return x <= 1 ? 1ull : 1ull << (64 - __builtin_clzll(x - 1));
}