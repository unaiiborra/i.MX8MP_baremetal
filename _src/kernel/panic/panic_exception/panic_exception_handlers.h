#pragma once

#include "kernel/panic.h"


typedef struct {
	uint64	esr;
	uint64	elr;
	uint64	far;
	uint64	spsr;
} exception_reason_sysregs;


void handle_sync_panic(panic_exception_src src);

void handle_irq_panic(panic_exception_src src);

void handle_fiq_panic(panic_exception_src src);

void handle_serror_panic(panic_exception_src src);


void print_esr(exception_reason_sysregs *r, panic_exception_type type);
