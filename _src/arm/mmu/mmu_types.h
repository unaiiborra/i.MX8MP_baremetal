#pragma once
#include <lib/mem.h>
#include <lib/stdbool.h>
#include <lib/stdint.h>

/// MMU page descriptor
typedef struct {
	uint64 v;
} mmu_hw_dc;


/// MMU table
typedef struct {
	mmu_hw_dc *dcs;
} mmu_tbl;


typedef enum {
	MMU_TBL_LV0 = 0,
	MMU_TBL_LV1,
	MMU_TBL_LV2,
	MMU_TBL_LV3,
} mmu_tbl_level;


typedef enum {
	MMU_DESCRIPTOR_BLOCK,
	MMU_DESCRIPTOR_TABLE,
	MMU_DESCRIPTOR_PAGE,
} mmu_descriptor_type;


typedef enum {
	MMU_SHAREABILITY_NON_SHAREABLE		= 0b00,
	MMU_SHAREABILITY_RESERVED		= 0b01,
	MMU_SHAREABILITY_OUTER_SHAREABLE	= 0b10,
	MMU_SHAREABILITY_INNER_SHAREABLE	= 0b11,
} mmu_shareability;


#define max_level(g)    (g == MMU_GRANULARITY_64KB ? MMU_TBL_LV2 : MMU_TBL_LV3)
