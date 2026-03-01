#pragma once

#include <lib/stdbool.h>
#include <lib/stdint.h>

/*
 *  ! Not thread safe by itself
 */

typedef struct {
	uint64	reg_id;
	uint32	span;
} xalloc_block_metadata;


typedef struct {
	uintptr			_region_start;
	size_t			_block_size;    //  in bytes
	size_t			_block_n;
	uint64			_reg_id_n;      // represents the next to be asigned region id
	xalloc_block_metadata * _blocks_metadata;
} xalloc_handle;

/// xalloc_block_metadata should be an array of block_n
bool xalloc_init(xalloc_handle *handle, uintptr region_start, size_t block_size, size_t block_n, xalloc_block_metadata blocks_metadata[]);

/// in bytes
size_t xalloc_get_block_size(xalloc_handle *handle);

/// returns the size of a region in blocks
size_t xalloc_get_region_blocks(xalloc_handle *handle, void *region);

/// gets the id of a region address. returns false if not found
bool xalloc_get_region_id(xalloc_handle *handle, void *region, uint64 *id);


/// returns the size of a region in bytes
size_t xalloc_get_region_bytes(xalloc_handle *handle, void *region);

/// allocates a region with size block_n * block_size and assigns it a reg_id
void * xalloc_alloc(xalloc_handle *handle, uint64 *reg_id, size_t block_n);

/// allocates a region with size block_n * block_size and assigns it a reg_id and initializes it to
/// zero
void * xalloc_calloc(xalloc_handle *handle, uint64 *reg_id, size_t block_n);

/// frees a region
void xalloc_free(xalloc_handle *handle, void *region);

/// frees a region by id
void xalloc_free_id(xalloc_handle *handle, uint64 region_id);
