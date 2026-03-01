#pragma once
#include <lib/stdint.h>


typedef struct {
	const char *	tag;
	// TODO: use a bitfield
	bool		device_mem;
	bool		permanent;
	uint8		cache_size; // saved as (log2(cache_size)).
} mm_page_data;


#define UNINIT_PAGE    (mm_page_data) { 0 }
