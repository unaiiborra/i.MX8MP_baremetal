#include <kernel/panic.h>
#include <lib/malloc/xalloc.h>
#include <lib/stdmacros.h>

#include "lib/stdint.h"


#define EMPTY_BLOCK    UINT64_MAX
#define NOT_FOUND      -1


static inline bool idx_is_not_first_id_(xalloc_handle *handle, size_t i)
{
	return i == 0 ? false
		  : handle->_blocks_metadata[i - 1].reg_id == handle->_blocks_metadata[i].reg_id;
}

static void free_by_idx_(xalloc_handle *handle, isize_t i)
{
	if (i == NOT_FOUND)
		PANIC("xalloc_free: attempted to free an inexistent region");

	if (idx_is_not_first_id_(handle, i))
		PANIC("xalloc_free: not the region start");

	if (i < 0)
		PANIC("xalloc_free: invalid idx from search_reg_addr_idx");

	size_t span = (size_t)handle->_blocks_metadata[i].span;

	if (i + span > handle->_block_n)
		PANIC("xalloc_free");

	if (handle->_blocks_metadata[i].reg_id == EMPTY_BLOCK)
		PANIC("xalloc_free: double free");

#ifdef TEST
	uint64 first_id = handle->_blocks_metadata[i].reg_id;
#endif

	for (size_t j = i; j < i + span; j++) {
#ifdef TEST
		if (first_id != handle->_blocks_metadata[j].reg_id)
			PANIC("xalloc_free");
#endif

		handle->_blocks_metadata[j] = (xalloc_block_metadata) {
			.reg_id = EMPTY_BLOCK,
			.span = 1,
		};
	}
}


/// returns the idx of the first free region with the block size available or -1
static isize_t search_free_region_(xalloc_handle *handle, size_t block_n)
{
	if (block_n == 0 || block_n > handle->_block_n)
		return NOT_FOUND;

	xalloc_block_metadata *mdt = handle->_blocks_metadata;

	size_t i = 0;

i_loop:
	while (i < handle->_block_n) {
		// Empty block encountered
		if (mdt[i].reg_id == EMPTY_BLOCK) {
			// No more space available
			if (i + block_n > handle->_block_n)
				return NOT_FOUND;

			size_t j = 0;

			while (j < block_n) {
				if (mdt[i + j].reg_id != EMPTY_BLOCK) {
#ifdef TEST
					if (mdt[i].span == 0)
						PANIC("search_free_region: span of 0 found");
#endif
					i = i + j + mdt[i + j].span;
					goto i_loop;
				}

				j++;
			}

			// found block
			return i;
		}

#ifdef TEST
		if (mdt[i].span == 0)
			PANIC("search_free_region: span of 0 found");
#endif
		// Occupied block encountered
		i += mdt[i].span;
	}

	return NOT_FOUND;
}

static isize_t search_reg_id_(xalloc_handle *handle, uint64 region_id)
{
	if (region_id >= handle->_reg_id_n)
		return NOT_FOUND;

	xalloc_block_metadata *mdt = handle->_blocks_metadata;

	for (size_t i = 0; i < handle->_block_n; i += handle->_blocks_metadata[i].span)
		if (mdt[i].reg_id == region_id)
			return (isize_t)i;
#ifdef TEST
	else if (handle->_blocks_metadata[i].span == 0 || i + mdt[i].span > handle->_block_n)
		PANIC("search_reg_id: invalid span found");
#endif

	return NOT_FOUND;
}

static inline isize_t search_reg_addr_idx_(xalloc_handle *handle, void *region)
{
	uintptr addr = (uintptr)region;

	if (!handle || !region)
		return NOT_FOUND;

	if (handle->_block_size == 0)
		return NOT_FOUND;

	if (addr < handle->_region_start)
		return NOT_FOUND;

	uintptr offset = addr - handle->_region_start;

	size_t i = offset / handle->_block_size;

	if (idx_is_not_first_id_(handle, i))
		return NOT_FOUND;

	if (offset % handle->_block_size != 0)
		return NOT_FOUND;

	if (i >= handle->_block_n)
		return NOT_FOUND;

	return (isize_t)i;
}


bool xalloc_init(xalloc_handle *handle, uintptr region_start, size_t block_size, size_t block_n,
		 xalloc_block_metadata blocks_metadata[])
{
	if (!handle || !blocks_metadata)
		return false;

	// blocks must be aligned to block size
	if (region_start % block_size != 0)
		return false;

	// only UINT64_MAX - 1 blocks can be used
	if (block_n >= UINT64_MAX)
		return false;

	*handle = (xalloc_handle) {
		._region_start = region_start,
		._block_size = block_size,
		._block_n = block_n,
		._reg_id_n = 0,
		._blocks_metadata = blocks_metadata,
	};

	for (size_t i = 0; i < block_n; i++)
		handle->_blocks_metadata[i] = (xalloc_block_metadata) {
			.reg_id = EMPTY_BLOCK,
			.span = 1,
		};

	return true;
}


size_t xalloc_get_block_size(xalloc_handle *handle)
{
	return handle->_block_size;
}

size_t xalloc_get_region_blocks(xalloc_handle *handle, void *region)
{
	isize_t i = search_reg_addr_idx_(handle, region);

	if (i == NOT_FOUND)
		return 0;

	return handle->_blocks_metadata[i].span;
}

bool xalloc_get_region_id(xalloc_handle *handle, void *region, uint64 *id)
{
	isize_t i = search_reg_addr_idx_(handle, region);

	if (i == NOT_FOUND)
		return false;

	*id = i;
	return true;
}


size_t xalloc_get_region_bytes(xalloc_handle *handle, void *region)
{
	return xalloc_get_region_blocks(handle, region) * handle->_block_size;
}


void * xalloc_alloc(xalloc_handle *handle, uint64 *reg_id, size_t block_n)
{
	if (!handle || !block_n)
		return NULL;

	if (!reg_id)
		return NULL;

	if (handle->_reg_id_n == EMPTY_BLOCK - 1)
		return NULL;

	isize_t region_idx = search_free_region_(handle, block_n);

	if (region_idx == NOT_FOUND || region_idx < 0) {
		*reg_id = (uint64) - 1;
		return NULL;
	}

	*reg_id = handle->_reg_id_n++;

	for (size_t i = 0; i < block_n; i++)
		handle->_blocks_metadata[region_idx + i] = (xalloc_block_metadata) {
			.reg_id = *reg_id,
			.span = block_n - i,
		};

	uintptr malloc_address =
		handle->_region_start + ((uintptr)region_idx * (uintptr)handle->_block_size);

#ifdef TEST
	if (malloc_address % (uintptr)handle->_block_size != 0 ||
	    malloc_address > (handle->_region_start + (handle->_block_n * handle->_block_size)))
		PANIC("xalloc_alloc failed");
#endif

	return (void *)malloc_address;
}


void * xalloc_calloc(xalloc_handle *handle, uint64 *reg_id, size_t block_n)
{
	void *addr = xalloc_alloc(handle, reg_id, block_n);

	if (!addr)
		return addr;

	// TODO: use memzero/memset
	uint8 *b = (uint8 *)addr;
	for (size_t i = 0; i < block_n * handle->_block_size; i++)
		b[i] = 0;

	return addr;
}


void xalloc_free(xalloc_handle *handle, void *region)
{
	isize_t i = search_reg_addr_idx_(handle, region);

	free_by_idx_(handle, i);
}


void xalloc_free_id(xalloc_handle *handle, uint64 region_id)
{
	isize_t i = search_reg_id_(handle, region_id);

	free_by_idx_(handle, i);
}
