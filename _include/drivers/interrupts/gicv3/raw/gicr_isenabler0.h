#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <kernel/panic.h>
#include <lib/mmio/mmio_macros.h>
#include <lib/stdbool.h>
#include <lib/stdint.h>

#include "gicv3_macros.h"

#define GICR_ISENABLER_OFFSET               0x100UL

#define GICR_ISENABLER_VALUE_STRUCT_NAME    GicrIsenabler0

static inline void GICV3_GICR_ISENABLER0_set_bit(uintptr base, uint32 cpu, uint32 intid)
{
	if (intid > 31)
		PANIC("GICD_ISENABLER: bit must be <= 31");

	*((reg32_ptr)(GICV3_SGI_BASE(base, cpu) + GICR_ISENABLER_OFFSET)) = (1UL << intid);
}
