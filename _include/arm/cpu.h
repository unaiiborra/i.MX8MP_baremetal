#pragma once

#include <lib/stdint.h>

typedef struct {
	_Alignas(4) uint8 aff3;
	uint8	aff2;
	uint8	aff1;
	uint8	aff0;
} ARM_cpu_affinity;

ARM_cpu_affinity ARM_get_cpu_affinity();

#define CPU_AFFINITY_FROM_U32(x)              \
	((ARM_cpu_affinity) {                 \
		.aff0 = (uint8)((x) & 0xFF),         \
		.aff1 = (uint8)(((x) >> 8) & 0xFF),  \
		.aff2 = (uint8)(((x) >> 16) & 0xFF), \
		.aff3 = (uint8)(((x) >> 24) & 0xFF), \
	})

#define U32_FROM_CPU_AFFINITY(x)                                    \
	(uint32)(                                                  \
		((uint32)(x).aff3 << 24) | ((uint32)(x).aff2 << 16) | \
		((uint32)(x).aff1 << 8) | ((uint32)(x).aff0))

extern uint64 _ARM_get_sp();
