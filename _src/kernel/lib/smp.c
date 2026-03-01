#include <arm/cpu.h>
#include <arm/tfa/smccc.h>
#include <kernel/lib/smp.h>
#include <kernel/panic.h>

#include "lib/stdint.h"

uint64 get_core_id()
{
	ARM_cpu_affinity affinity = ARM_get_cpu_affinity();

#ifdef TEST
	if (!(affinity.aff0 < 4))
		PANIC("Invalid core id");
#endif

	return affinity.aff0;
}


bool wake_core(uint64 core_id, uintptr entry_addr, uint64 context)
{
	smccc_res_t res =
		_smc_call(PSCI_CPU_ON_FID64, core_id, entry_addr, context, 0x0, 0x0, 0x0, 0x0);

	if (res.x0 == 0)
		return true;

	return false;
}
