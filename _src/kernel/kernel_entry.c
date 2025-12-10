#include <drivers/uart/uart.h>
#include <kernel/panic.h>
#include <lib/kernel_utils.h>
#include <lib/stdint.h>
#include <lib/stdmacros.h>
#include <lib/string.h>

#include "lib/memcpy.h"

extern void rust_test_panic();

static void kernel_init()
{
	init_panic();
	UART_init(UART_ID_2);
}

static inline uint64 read_id_aa64pfr0_el1(void)
{
	uint64 v;
	__asm__ volatile("mrs %0, ID_AA64PFR0_EL1" : "=r"(v));
	return v;
}

__attribute__((aligned(64))) static uint64 MEMCPY_TEST_SRC[8 * 10];
__attribute__((aligned(64))) static uint64 MEMCPY_TEST_DST[8 * 10];

void kernel_entry()
{
	kernel_init();

	UART_putc(UART_ID_2, (uint8)(_currentEL() + 48));

	UART_puts(UART_ID_2, "Hello world\n\r");

	for (uint64 i = 0; i < 8 * 10; i++) {
		MEMCPY_TEST_SRC[i] = i;
	}

	memcpy64(MEMCPY_TEST_DST, MEMCPY_TEST_SRC, 8 * 10 * 8);

	char buf[200];
	for (uint64 i = 0; i < 8 * 10; i++) {
		UART_puts(UART_ID_2,
				  stdint_to_ascii((STDINT_UNION){.uint64 = MEMCPY_TEST_DST[i]},
								  STDINT_UINT64, buf, 200, STDINT_REPR_HEX));
	}

	FOREVER {}

	uint64 v = (read_id_aa64pfr0_el1() >> 20 & 0xF);

	UART_puts(UART_ID_2,
			  stdint_to_ascii((STDINT_UNION){.uint64 = v}, STDINT_UINT64, buf,
							  200, STDINT_REPR_HEX));

	UART_init(UART_ID_2);

	//	UART_puts(UART_ID_2, "Hello world :)\n\r");

	FOREVER
	{
		// UART_puts(UART_ID_2, "Hello world :)\n\r");

		for (volatile int i = 0; i < 20000; i++) {
			asm volatile("nop");
			asm volatile("nop");
			asm volatile("nop");
			asm volatile("nop");
			asm volatile("nop");
		}
	}
}