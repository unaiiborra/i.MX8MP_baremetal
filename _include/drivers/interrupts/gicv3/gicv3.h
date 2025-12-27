#pragma once

#include <arm/cpu.h>
#include <kernel/irq/interrupts.h>
#include <lib/stdint.h>

#include "kernel/devices/device.h"

void GICV3_route_spi_to_cpu(const driver_handle *h, imx8mp_irq irq,
							ARM_cpu_affinity affinity);

void GICV3_route_spi_to_self(const driver_handle *h, imx8mp_irq irq);

void GICV3_enable_spi(const driver_handle *h, imx8mp_irq irq);

void GICV3_set_priority(const driver_handle *h, imx8mp_irq irq, uint8 priority);

void GICV3_set_edge_triggered(const driver_handle *h, imx8mp_irq irq);
void GICV3_set_level_sensitive(const driver_handle *h, imx8mp_irq irq);

void GICV3_wake_redistributor(const driver_handle *h, size_t n);

void GICV3_init_distributor(const driver_handle *h);

void GICV3_init_cpu(const driver_handle *h, size_t cpu);

void GICV3_set_cpu_priority_threshold(uint8 threshold);

typedef enum {
	GICV3_LEVEL_SENSITIVE,
	GICV3_EDGE_TRIGGERED,
} gicv3_irq_trigger;

uint64 GICV3_get_intid_el1();
imx8mp_irq GICV3_imx8mp_irq_from_intid(uint64 intid);
void GICV3_ack_intid_el1(uint64 irq_token);

void GICV3_init_irq(const driver_handle *h, imx8mp_irq irq, uint8 priority,
					gicv3_irq_trigger trigger, ARM_cpu_affinity cpu);