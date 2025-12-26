// The purpose of this file is to manually set the initialization stages of the
// drivers, as a driver can need to initialize before and after irqs are enabled

#include <drivers/interrupts/gicv3/gicv3.h>
#include <drivers/uart/uart.h>
#include <kernel/init.h>

#include "arm/cpu.h"
#include "drivers/interrupts/interrupts.h"

static void uart_stage0()
{
	for (UART_ID id = UART_ID_1; id <= UART_ID_4; id++) {
		UART_init_stage0(id);
	}
}

static void uart_stage1()
{
	// UART_init_stage1(UART_ID_1);
	UART_init_stage1(UART_ID_2);
	// UART_init_stage1(UART_ID_3);
	// UART_init_stage1(UART_ID_4);
}

static void uart_stage2()
{
	GICV3_init_irq(IMX8MP_IRQ_UART2, 0x80, GICV3_LEVEL_SENSITIVE,
				   ARM_get_cpu_affinity());
}

KERNEL_INITCALL(uart_stage0, KERNEL_INITCALL_STAGE0);
KERNEL_INITCALL(uart_stage1, KERNEL_INITCALL_STAGE1);
KERNEL_INITCALL(uart_stage2, KERNEL_INITCALL_STAGE2);
