#include <arm/exceptions/handlers/handlers_macros.h>
#include <drivers/interrupts/gicv3/gicv3.h>
#include <drivers/uart/uart.h>
#include <kernel/irq/interrupts.h>
#include <kernel/irq/irq.h>
#include <lib/string.h>

void el1_cur_spx_irq_handler(void)
{
	irq_id intid = GICV3_get_intid_el1();

	kernel_handle_irq(intid);

	GICV3_ack_intid_el1(intid);
}
