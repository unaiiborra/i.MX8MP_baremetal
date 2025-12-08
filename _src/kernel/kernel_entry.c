#include <drivers/uart/uart.h>
#include <kernel/panic.h>
#include <lib/kernel_utils.h>
#include <lib/stdmacros.h>

#include "lib/stdint.h"

extern void rust_panic_test();

void kernel_entry()
{
	init_panic();

	UART_putc(UART_ID_2, (uint8)(_currentEL() + 48));

	UART_puts(UART_ID_2, "Hello world\n\r");

	// UART_reset(UART_ID_2);

	PANIC(Panic test from C !);
	rust_panic_test();	// panic test

	UART_init(UART_ID_2);

	//	UART_puts(UART_ID_2, "Hello world :)\n\r");

	FOREVER {}

	FOREVER
	{
		UART_puts(UART_ID_2, "Hello world :)\n\r");

		for (volatile int i = 0; i < 20000; i++) {
			asm volatile("nop");
			asm volatile("nop");
			asm volatile("nop");
			asm volatile("nop");
			asm volatile("nop");
		}
	}
}