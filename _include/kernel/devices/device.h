#pragma once
// This header defines an abstraction interface for drivers, where the kernel
// does not have to know the internals of each driver

#include <lib/stdint.h>

typedef struct {
	uintptr base;
	void *	state;
} driver_handle;

typedef struct {
	uint64			id;
	const char *		name;
	uint64			irqid;
	const driver_handle *	drv;
} kernel_device;
