#include <kernel/panic.h>
#include <lib/memcpy.h>

#include "lib/stdint.h"

extern void *_memcpy64(void *dst, void *src, uint64 size);
extern void *_memcpy32(void *dst, void *src, uint64 size);
extern void *_memcpy16(void *dst, void *src, uint64 size);
extern void *_memcpy8(void *dst, void *src, uint64 size);
extern void *_memcpy4(void *dst, void *src, uint64 size);
extern void *_memcpy1(void *dst, void *src, uint64 size);

// byte per byte
// extern void *_memcpy(void *dst, void *src, uint64 size);

void *memcpy64_aligned(void *dst, void *src, uint64 size)
{
	if (size == 0) return dst;

	if (((uintptr)dst & 63) != 0)
		PANIC("memcpy64_aligned: dst not aligned to 64 bytes");

	if (((uintptr)src & 63) != 0)
		PANIC("memcpy64_aligned: src not aligned to 64 bytes");

	if ((size & 63) != 0)
		PANIC("memcpy64_aligned: size is not a multiple of 64");

	return _memcpy64(dst, src, size);
}

void *memcpy64(void *dst, void *src, uint64 size)
{
	if (size == 0) return dst;

	if ((size & 63) != 0) PANIC("memcpy64: size is not a multiple of 64");

	return _memcpy64(dst, src, size);
}

/*
void *memcpy(void *dst, void *src, uint64 size)
{

	return dst;
}
*/