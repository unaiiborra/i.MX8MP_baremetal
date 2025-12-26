#pragma once
#include <lib/stdbitfield.h>
#include <lib/stdbool.h>
#include <lib/stdint.h>

typedef enum {
	UART_ID_1 = 0,
	UART_ID_2,
	UART_ID_3,
	UART_ID_4,

	UART_ID_COUNT,
} UART_ID;

void UART_reset(UART_ID id);

// Pre IRQ initialization
void UART_init_stage0(UART_ID id);

// Post IRQ initialization
void UART_init_stage1(UART_ID id);

bool UART_read(UART_ID id, uint8 *data);

// The kernel should call this fn
void UART_handle_irq(UART_ID id);

void UART_putc_sync(UART_ID id, const uint8 c);
void UART_puts_sync(UART_ID id, const char *s);

void UART_putc(UART_ID id, const uint8 c);
void UART_puts(const UART_ID id, const char *s);