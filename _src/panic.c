#include <drivers/uart/uart.h>
#include <kernel/panic.h>
#include <lib/stdmacros.h>
#include <lib/string.h>

#include "lib/stdint.h"

#define PANIC_UART_OUTPUT UART_ID_2

#define PANIC_MESSAGE_INIT_VALUE 4096
#define PANIC_FILE_INIT_VALUE 1024

// The global static scope variables and buffers allow rust to easily send the
// message in a c format string with \0. It also allows to set the panic
// information without throwing the panic.

uint64 PANIC_MESSAGE_LEN;
uint64 PANIC_FILE_LEN;

// TODO: If implementing multithreading the panic infos must be protected by a
// mutex

uint8 *PANIC_MESSAGE_PTR;
uint8 *PANIC_FILE_PTR;
uint32 PANIC_LINE;
uint32 PANIC_COL;

static uint8 panic_message_buffer[PANIC_MESSAGE_INIT_VALUE];
static uint8 panic_file_buffer[PANIC_FILE_INIT_VALUE];

void init_panic()
{
	PANIC_MESSAGE_LEN = PANIC_MESSAGE_INIT_VALUE;
	PANIC_FILE_LEN = PANIC_FILE_INIT_VALUE;

	PANIC_MESSAGE_PTR = panic_message_buffer;
	PANIC_FILE_PTR = panic_file_buffer;

	PANIC_LINE = 0;
	PANIC_COL = 0;

	strcopy((char *)PANIC_MESSAGE_PTR,
			"Panic message not defined and not changed from init_panic() "
			"initialization stage.",
			PANIC_MESSAGE_INIT_VALUE);

	strcopy((char *)PANIC_FILE_PTR, "Panic file not defined",
			PANIC_FILE_INIT_VALUE);
}

void panic()
{
	UART_puts(PANIC_UART_OUTPUT, "\n\r!!![PANIQUED]!!!\n\r[Panic file]\n\r");
	UART_puts(PANIC_UART_OUTPUT, (char *)PANIC_FILE_PTR);
	UART_puts(PANIC_UART_OUTPUT, " at line ");
	// TODO: atoi
	UART_puts(PANIC_UART_OUTPUT, " at column ");

	UART_puts(PANIC_UART_OUTPUT, "\n\r[Panic message]\n\r");
	UART_puts(PANIC_UART_OUTPUT, (char *)PANIC_MESSAGE_PTR);
	UART_puts(PANIC_UART_OUTPUT, "\n\r");

	FOREVER {}
}

void set_panic(PanicInfo panic_info)
{
	strcopy((char *)PANIC_MESSAGE_PTR, panic_info.message,
			PANIC_MESSAGE_INIT_VALUE);

	strcopy((char *)PANIC_FILE_PTR, panic_info.location.file,
			PANIC_FILE_INIT_VALUE);

	PANIC_LINE = panic_info.location.line;
	PANIC_COL = panic_info.location.col;
}

void set_and_throw_panic(PanicInfo panic_info)
{
	set_panic(panic_info);
	panic();
}
