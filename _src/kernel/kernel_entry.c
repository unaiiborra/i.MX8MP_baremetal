#include <arm/exceptions/exceptions.h>
#include <boot/panic.h>
#include <drivers/interrupts/gicv3/gicv3.h>
#include <drivers/uart/uart.h>
#include <kernel/init.h>
#include <lib/memcpy.h>
#include <lib/stdint.h>
#include <lib/stdmacros.h>
#include <lib/string.h>

#include "arm/cpu.h"
#include "drivers/arm_generic_timer/arm_generic_timer.h"
#include "kernel/devices/drivers.h"

static inline uint64 sec_to_ns(uint64 sec) { return sec * 1'000'000'000ULL; }

static void testcb(void *)
{
	UART_puts_sync(&UART2_DRIVER, "a\n");
	AGT_timer_schedule_delta(&AGT0_DRIVER, sec_to_ns(1), testcb, NULL);
}

// Main function of the kernel, called by the bootloader (/boot/boot.S)
_Noreturn void kernel_entry()
{
	kernel_init();

	GICV3_enable_ppi(&GIC_DRIVER, irq_id_new(27), ARM_get_cpu_affinity());

	UART_puts(&UART2_DRIVER, "Hello world!\n\r");

	char buf1[100];

	uint8 data;
	while (1) {
		if (UART_read(&UART2_DRIVER, &data)) {
			uint64 time = AGT_cnt_time_us();

			stdint_to_ascii((STDINT_UNION){.uint64 = time}, STDINT_UINT64, buf1,
							100, STDINT_BASE_REPR_DEC);

			UART_puts(&UART2_DRIVER, buf1);
			UART_puts(&UART2_DRIVER, "\n\r");

			AGT_timer_schedule_delta(&AGT0_DRIVER, sec_to_ns(1), testcb, NULL);
		}
	}

	loop {}
}