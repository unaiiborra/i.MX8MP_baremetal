#pragma once

#include <lib/stdint.h>

typedef struct {
	int64	x0;
	int64	x1;
	int64	x2;
	int64	x3;
} smccc_res_t;

extern smccc_res_t _smc_call(uint64 fid, uint64 arg1, uint64 arg2, uint64 arg3, uint64 arg4, uint64 arg5, uint64 arg6, uint64 arg7);

#define ARM_SMCCC_VERSION_FID      0x80000000u
#define SMCCC_ARCH_FEATURES_FID    0x80000001u
#define PSCI_VERSION_FID           0x84000000u
#define PSCI_SYSTEM_OFF_FID        0x84000008u
#define PSCI_CPU_ON_FID64          0x84000003u
#define PSCI_VERSION               0x84000000u

uint32 tfa_get_smccc_version(void);
