#pragma once

#ifndef DRIVERS
#error "This header should only be imported by a driver"
#endif

#include <boot/panic.h>
#include <lib/mmio/mmio_macros.h>
#include <lib/stdbool.h>
#include <lib/stdint.h>

#define GICD_ISENABLER_OFFSET(n) (0x100UL + (4UL * n))

static inline void GICV3_GICD_ISENABLER_set_bit(uintptr base, uint32 n,
												uint32 bit)
{
	if (bit > 31) PANIC("GICD_ISENABLER: bit must be <= 31");

	*((reg32_ptr)(base + GICD_ISENABLER_OFFSET(n))) = (1UL << bit);
}