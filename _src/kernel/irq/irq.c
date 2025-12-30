#include <boot/panic.h>
#include <drivers/uart/uart.h>
#include <kernel/devices/drivers.h>
#include <kernel/init.h>
#include <kernel/irq/interrupts.h>
#include <kernel/irq/irq.h>
#include <lib/stdint.h>

#include "drivers/interrupts/gicv3/gicv3.h"
#include "drivers/tmu/tmu.h"

typedef void (*irq_handler_t)(void);

static irq_handler_t KERNEL_IRQ_HANDLER_TABLE[IMX8MP_IRQ_SIZE];

static void unhandled_irq() { PANIC("UNHANDLED IRQ"); }

static void UART1_handler() { UART_handle_irq(&UART2_DRIVER); }
static void UART2_handler() { UART_handle_irq(&UART2_DRIVER); }
static void UART3_handler() { UART_handle_irq(&UART2_DRIVER); }
static void UART4_handler() { UART_handle_irq(&UART2_DRIVER); }
static void TMU_handler() { TMU_handle_irq(&TMU_DRIVER); }

static void init_irq_handler_table()
{
	for (size_t i = 0; i < IMX8MP_IRQ_SIZE; i++) {
		KERNEL_IRQ_HANDLER_TABLE[i] = unhandled_irq;
	}

	KERNEL_IRQ_HANDLER_TABLE[IMX8MP_IRQ_UART1] = UART1_handler;
	KERNEL_IRQ_HANDLER_TABLE[IMX8MP_IRQ_UART2] = UART2_handler;
	KERNEL_IRQ_HANDLER_TABLE[IMX8MP_IRQ_UART3] = UART3_handler;
	KERNEL_IRQ_HANDLER_TABLE[IMX8MP_IRQ_UART4] = UART4_handler;
	KERNEL_IRQ_HANDLER_TABLE[IMX8MP_IRQ_ANAMIX_TEMP] = TMU_handler;
}

KERNEL_INITCALL(init_irq_handler_table, KERNEL_INITCALL_STAGE0);

void kernel_handle_irq(irq_id irq) { KERNEL_IRQ_HANDLER_TABLE[irq.n](); }
