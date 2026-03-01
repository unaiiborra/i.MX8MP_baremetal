#include <arm/cpu.h>
#include <arm/sysregs/sysregs.h>

#include "lib/stdint.h"

ARM_cpu_affinity ARM_get_cpu_affinity()
{
	uint64 v = _ARM_MPIDR_EL1();
	ARM_cpu_affinity cpuid = {
		.aff0	= (v & 0xFFUL),
		.aff1	= ((v >> 8) & 0xFFUL),
		.aff2	= ((v >> 16) & 0xFFUL),
		.aff3	= ((v >> 32) & 0xFFUL),
	};

	return cpuid;
}
