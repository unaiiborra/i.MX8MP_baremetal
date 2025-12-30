#include <boot/panic.h>
#include <drivers/uart/uart.h>
#include <kernel/devices/drivers.h>
#include <kernel/init.h>
#include <kernel/irq/interrupts.h>
#include <kernel/irq/irq.h>
#include <lib/stdint.h>
#include <lib/stdmacros.h>

#include "drivers/arm_generic_timer/arm_generic_timer.h"
#include "drivers/interrupts/gicv3/gicv3.h"
#include "drivers/tmu/tmu.h"
#include "kernel/devices/device.h"

typedef void (*driver_irq_handler)(const driver_handle *h);

typedef struct {
	driver_irq_handler handler;
	const driver_handle *h;
} kernel_irq_handler;

static kernel_irq_handler KERNEL_IRQ_HANDLER_TABLE[IMX8MP_IRQ_SIZE];

static void unhandled_irq(const driver_handle *) { PANIC("UNHANDLED IRQ"); }

static inline kernel_irq_handler build_handler(driver_irq_handler handler,
											   const driver_handle *h)
{
	return (kernel_irq_handler){
		.handler = handler,
		.h = h,
	};
}

static void init_irq_handler_table()
{
	for (size_t i = 0; i < IMX8MP_IRQ_SIZE; i++) {
		KERNEL_IRQ_HANDLER_TABLE[i] = (kernel_irq_handler){unhandled_irq, NULL};
	}

	KERNEL_IRQ_HANDLER_TABLE[IMX8MP_IRQ_UART1] =
		build_handler(UART_handle_irq, &UART1_DRIVER);
	KERNEL_IRQ_HANDLER_TABLE[IMX8MP_IRQ_UART2] =
		build_handler(UART_handle_irq, &UART2_DRIVER);
	KERNEL_IRQ_HANDLER_TABLE[IMX8MP_IRQ_UART3] =
		build_handler(UART_handle_irq, &UART3_DRIVER);
	KERNEL_IRQ_HANDLER_TABLE[IMX8MP_IRQ_UART4] =
		build_handler(UART_handle_irq, &UART4_DRIVER);

	KERNEL_IRQ_HANDLER_TABLE[IMX8MP_IRQ_ANAMIX_TEMP] =
		build_handler(TMU_handle_irq, &TMU_DRIVER);

	KERNEL_IRQ_HANDLER_TABLE[27] = build_handler(AGT_handle_irq, &AGT0_DRIVER); // TODO: 0..31 irq enum
}

KERNEL_INITCALL(init_irq_handler_table, KERNEL_INITCALL_STAGE0);

void kernel_handle_irq(irq_id irq)
{
	KERNEL_IRQ_HANDLER_TABLE[irq.n].handler(KERNEL_IRQ_HANDLER_TABLE[irq.n].h);
}
