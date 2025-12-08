#pragma once
#include <lib/stdint.h>

typedef enum {
	UART_ID_1 = 0,
	UART_ID_2,
	UART_ID_3,
	UART_ID_4,
} UART_ID;

void UART_reset(UART_ID id);

void UART_init(UART_ID id);

void UART_putc(UART_ID id, uint8 c);

void UART_puts(const UART_ID id, const char *s);
