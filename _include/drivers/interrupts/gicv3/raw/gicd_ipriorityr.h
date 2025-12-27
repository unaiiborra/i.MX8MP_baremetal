#pragma once

#ifndef DRIVERS
#error "This header should only be imported by a driver"
#endif

#include <boot/panic.h>
#include <lib/mmio/mmio_macros.h>
#include <lib/stdbool.h>
#include <lib/stdint.h>

#include "gicv3_macros.h"

#define GICD_IPRIORITYR_OFFSET(n) (0x400UL + (4UL * n))

#define GICD_IPRIORITYR_VALUE_STRUCT_NAME GicdIpriority

MMIO_DECLARE_REG32_VALUE_STRUCT(GICD_IPRIORITYR_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_GETTER_N_OFFSET(GICV3, GICD_IPRIORITYR,
								   GICD_IPRIORITYR_VALUE_STRUCT_NAME,
								   GICD_IPRIORITYR_OFFSET);

MMIO_DECLARE_REG32_SETTER_N_OFFSET(GICV3, GICD_IPRIORITYR,
								   GICD_IPRIORITYR_VALUE_STRUCT_NAME,
								   GICD_IPRIORITYR_OFFSET);

static inline uint8 GICV3_GICD_IPRIORITYR_BF_get(
	const GICD_IPRIORITYR_VALUE_STRUCT_NAME r, size_t byte_idx)
{
	if (byte_idx > 3) PANIC("GICD_IPRIORITYR: byte_idx index must be <= 3");

	uint32 shift = (byte_idx * 8);

	return (uint8)((r.val >> shift) & 0xFFUL);
}

static inline void GICV3_GICD_IPRIORITYR_BF_set(
	GICD_IPRIORITYR_VALUE_STRUCT_NAME *r, size_t byte_idx, uint8 priority)
{
	if (byte_idx > 3) PANIC("GICD_IPRIORITYR: byte_idx index must be <= 3");

	uint32 shift = (byte_idx * 8);

	r->val =
		(r->val & ~(0xFFUL << shift)) | (((uint32)priority & 0xFFUL) << shift);
};