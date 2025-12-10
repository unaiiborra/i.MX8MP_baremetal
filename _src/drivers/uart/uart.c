#include <drivers/uart/uart.h>
#include <drivers/uart/uart_raw.h>
#include <lib/stdint.h>
#include <lib/stdmacros.h>


static const uintptr UART_N_BASE[] = {
	UART1_BASE,
	UART2_BASE,
	UART3_BASE,
	UART4_BASE,
};

// Tx fifo full
static inline bool UART_tx_fifo_full(uintptr periph_base)
{
	UartUtsValue uts = UART_UTS_read(periph_base);
	return UART_UTS_BF_get_TXFULL(uts);
}

void UART_putc(UART_ID id, uint8 c)
{
	UartUtxdValue utxd = {(uint32)c};

	while (UART_tx_fifo_full(UART_N_BASE[id])) {
		asm volatile("nop");
	}

	UART_UTXD_write(UART_N_BASE[id], utxd);
}

void UART_puts(const UART_ID id, const char *s)
{
	while (*s) UART_putc(id, *s++);
}

void UART_reset(UART_ID id)
{
	uintptr periph_base = UART_N_BASE[id];
	UartUcr2Value ucr2 = UART_UCR2_read(periph_base);
	UART_UCR2_BF_set_SRST(&ucr2, false);  // UART software reset 17.2.8.2

	FOREVER
	{
		for (uint64 i = 0; i < 2000; i++) asm volatile("nop");

		UartUtsValue uts = UART_UTS_read(periph_base);
		if (UART_UTS_BF_get_SOFTRST(uts) == 0) {
			break;
		}
	}
}

// FIXME: read the configuration left by the bootloader to watch what is wrong
void UART_init(UART_ID id)
{
	// 17.2.12.1 7357
	uintptr periph_base = UART_N_BASE[id];

	UartUcr1Value ucr1 = {0};
	UART_UCR1_BF_set_UARTEN(&ucr1, true);
	UART_UCR1_write(periph_base, ucr1);

	UartUcr2Value ucr2 = {0};
	UART_UCR2_BF_set_SRST(&ucr2, true);
	UART_UCR2_BF_set_RXEN(&ucr2, true);
	UART_UCR2_BF_set_TXEN(&ucr2, true);
	UART_UCR2_BF_set_WS(&ucr2, true);
	UART_UCR2_BF_set_IRTS(&ucr2, true);
	UART_UCR2_write(periph_base, ucr2);

	UartUcr3Value ucr3 = {0};
	UART_UCR3_BF_set_RXDMUXSEL(&ucr3, true);
	UART_UCR3_BF_set_RI(&ucr3, true);
	UART_UCR3_BF_set_DCD(&ucr3, true);
	UART_UCR3_BF_set_DSR(&ucr3, true);
	UART_UCR3_write(periph_base, ucr3);

	UartUcr4Value ucr4 = {0};
	UART_UCR4_BF_set_CTSTL(&ucr4, 31);
	UART_UCR4_write(periph_base, ucr4);

	UartUfcrValue ufcr = {0xA01};  // 0000 1010 0000 0001
	UART_UFCR_write(periph_base, ufcr);

	UartUbirValue ubir = {0};
	UART_UBIR_BF_set_INC(&ubir, 0xF);
	UART_UBIR_write(periph_base, ubir);

	UartUbmrValue ubmr = {0};
	UART_UBMR_BF_set_MOD(&ubmr, 0x68);
	UART_UBMR_write(periph_base, ubmr);

	UartUmcrValue umcr = {0};
	UART_UMCR_write(periph_base, umcr);
}
