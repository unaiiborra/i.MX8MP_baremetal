#pragma once

#include <lib/stdint.h>

extern uint64 _ARM_currentEL();

extern uint64 _ARM_MPIDR_EL1();

extern uint64 _ARM_ICC_SRE_EL2();

extern uint64 _ARM_HCR_EL2();

extern uint64 _ARM_ESR_EL2();
extern uint64 _ARM_ELR_EL2();
extern uint64 _ARM_FAR_EL2();
extern uint64 _ARM_SPSR_EL2();

extern uint64 _ARM_ESR_EL1();
extern uint64 _ARM_ELR_EL1();
extern uint64 _ARM_FAR_EL1();
extern uint64 _ARM_SPSR_EL1();

extern uint64 _ARM_SCTLR_EL1();
extern uint64 _ARM_SCTLR_EL2();
