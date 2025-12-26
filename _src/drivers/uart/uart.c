#include <boot/panic.h>
#include <drivers/uart/uart.h>
#include <drivers/uart/uart_raw.h>
#include <lib/stdint.h>

// Rust fns (driver buffer control)
extern bool UART_txbuf_push(UART_ID id, uint8 v);
extern bool UART_rxbuf_push(UART_ID id, uint8 v);
extern bool UART_txbuf_pop(UART_ID id, uint8 *v);
extern bool UART_rxbuf_pop(UART_ID id, uint8 *v);

static const uintptr UART_N_BASE[] = {
	UART1_BASE,
	UART2_BASE,
	UART3_BASE,
	UART4_BASE,
};

// Saves which irqs are enabled or disabled, each bitfield represents each
// UART_ID
static bitfield32 UART_IRQ_STATE[UART_ID_COUNT];

static const uint8 USR1_IRQ_W1C_BITS[9] = {
	3, 4, 5, 7, 8, 10, 11, 12, 15,
};
static const uint8 USR2_IRQ_W2C_BITS[8] = {
	1, 2, 4, 7, 8, 11, 12, 15,
};

// Tx fifo full
static inline bool UART_tx_fifo_full(uintptr periph_base)
{
	UartUtsValue uts = UART_UTS_read(periph_base);
	return UART_UTS_BF_get_TXFULL(uts);
}

void UART_reset(UART_ID id)
{
	// FIXME:

	uintptr base = UART_N_BASE[id];

	UART_UCR2_write(base, (UartUcr2Value){.val = 0});

	while (!(UART_UCR2_read(base).val & (1 << 0)));
}

bool UART_read(UART_ID id, uint8 *data) { return UART_rxbuf_pop(id, data); }

/*
	------------
	IRQ HANDLING
	------------
*/

typedef enum {
	UART_IRQ_SRC_START = 0,

	/* =========================================================
	 * RX / Receiver related
	 * ========================================================= */

	/* RRDYEN (UCR1[9])  -> RRDY (USR1[9]) */
	UART_IRQ_SRC_RRDY = 0,	// Rx FIFO ready / threshold reached

	/* IDEN (UCR1[12])  -> IDLE (USR2[12]) */
	UART_IRQ_SRC_IDLE,	// Idle line detected

	/* DREN (UCR4[0])   -> RDR (USR2[0]) */
	UART_IRQ_SRC_RDR,  // DMA receive request

	/* RXDSEN (UCR3[6]) -> RXDS (USR1[6]) */
	UART_IRQ_SRC_RXDS,	// Rx data size

	/* ATEN (UCR2[3])   -> AGTIM (USR1[8]) */
	UART_IRQ_SRC_AGTIM,	 // Aging timer expired

	/* =========================================================
	 * TX / Transmitter related
	 * ========================================================= */

	/* TXMPTYEN (UCR1[6]) -> TXFE (USR2[14]) */
	UART_IRQ_SRC_TXFE,	// Tx FIFO empty

	/* TRDYEN (UCR1[13]) -> TRDY (USR1[13]) */
	UART_IRQ_SRC_TRDY,	// Tx FIFO ready

	/* TCEN (UCR4[3])   -> TXDC (USR2[3]) */
	UART_IRQ_SRC_TXDC,	// Transmit complete

	/* =========================================================
	 * Error / Modem / Control
	 * ========================================================= */

	/* OREN (UCR4[1])   -> ORE (USR2[1]) */
	UART_IRQ_SRC_ORE,  // Overrun error

	/* BKEN (UCR4[2])   -> BRCD (USR2[2]) */
	UART_IRQ_SRC_BRCD,	// Break detected

	/* WKEN (UCR4[7])   -> WAKE (USR2[7]) */
	UART_IRQ_SRC_WAKE,	// Wake-up event

	/* ADEN (UCR1[15])  -> ADET (USR2[15]) */
	UART_IRQ_SRC_ADET,	// Auto-baud detect

	/* ACIEN (UCR3[0])  -> ACST (USR2[11]) */
	UART_IRQ_SRC_ACST,	// Auto-baud complete

	/* ESCI (UCR2[15])  -> ESCF (USR1[11]) */
	UART_IRQ_SRC_ESC,  // Escape sequence

	/* ENIRI (UCR4[8])  -> IRINT (USR2[8]) */
	UART_IRQ_SRC_IRINT,	 // IR interrupt

	/* AIRINTEN (UCR3[5]) -> AIRINT (USR1[5]) */
	UART_IRQ_SRC_AIRINT,  // Auto IR interrupt

	/* AWAKEN (UCR3[4]) -> AWAKE (USR1[4]) */
	UART_IRQ_SRC_AWAKE,	 // Auto wake

	/* FRAERREN (UCR3[11]) -> FRAERR (USR1[10]) */
	UART_IRQ_SRC_FRAERR,  // Framing error

	/* PARERREN (UCR3[12]) -> PARITYERR (USR1[15]) */
	UART_IRQ_SRC_PARITYERR,	 // Parity error

	/* RTSDEN (UCR1[5]) -> RTSD (USR1[12]) */
	UART_IRQ_SRC_RTSD,	// RTS delta

	/* RTSEN (UCR2[4])  -> RTSF (USR2[4]) */
	UART_IRQ_SRC_RTSF,	// RTS edge

	/* DTREN (UCR3[13]) -> DTRF (USR2[13]) */
	UART_IRQ_SRC_DTRF,	// DTR edge

	/* RI (DTE) (UCR3[8]) -> RIDELT (USR2[10]) */
	UART_IRQ_SRC_RIDELT,  // Ring indicator delta

	/* DCD (DTE) (UCR3[9]) -> DCDDELT (USR2[6]) */
	UART_IRQ_SRC_DCDDELT,  // DCD delta

	/* DTRDEN (UCR3[3]) -> DTRD (USR1[7]) */
	UART_IRQ_SRC_DTRD,	// DTR delta detect

	/* SADEN (UMCR[3])  -> SAD (USR1[3]) */
	UART_IRQ_SRC_SAD,  // Stop-after-detect

	UART_IRQ_SRC_COUNT
} UART_IRQ_SOURCE;

/// Enables/Disables the irq
static void UART_set_irq_state(UART_ID id, UART_IRQ_SOURCE irq, bool state)
{
#ifdef TEST
	if (irq >= UART_IRQ_SRC_COUNT) {
		PANIC("invalid input");
	}
#endif

	if (state)
		BITFIELD32_SET(&(UART_IRQ_STATE[id]), irq);
	else
		BITFIELD32_CLEAR(&(UART_IRQ_STATE[id]), irq);

	uintptr periph_base = UART_N_BASE[id];

#define SET_IRQ_CASE(irq, reg, bf, regv_name)         \
	case irq: {                                       \
		regv_name r = UART_##reg##_read(periph_base); \
		UART_##reg##_BF_set_##bf(&r, state);          \
		UART_##reg##_write(periph_base, r);           \
	} break;

	switch (irq) {
		/* ================= RX ================= */
		SET_IRQ_CASE(UART_IRQ_SRC_RRDY, UCR1, RRDYEN, UartUcr1Value);
		SET_IRQ_CASE(UART_IRQ_SRC_IDLE, UCR1, IDEN, UartUcr1Value);
		SET_IRQ_CASE(UART_IRQ_SRC_RDR, UCR4, DREN, UartUcr4Value);
		SET_IRQ_CASE(UART_IRQ_SRC_RXDS, UCR3, RXDSEN, UartUcr3Value);
		SET_IRQ_CASE(UART_IRQ_SRC_AGTIM, UCR2, ATEN, UartUcr2Value);

		/* ================= TX ================= */
		SET_IRQ_CASE(UART_IRQ_SRC_TXFE, UCR1, TXMPTYEN, UartUcr1Value);
		SET_IRQ_CASE(UART_IRQ_SRC_TRDY, UCR1, TRDYEN, UartUcr1Value);
		SET_IRQ_CASE(UART_IRQ_SRC_TXDC, UCR4, TCEN, UartUcr4Value);

		/* ========== Error / Modem / Control ========== */
		SET_IRQ_CASE(UART_IRQ_SRC_ORE, UCR4, OREN, UartUcr4Value);
		SET_IRQ_CASE(UART_IRQ_SRC_BRCD, UCR4, BKEN, UartUcr4Value);
		SET_IRQ_CASE(UART_IRQ_SRC_WAKE, UCR4, WKEN, UartUcr4Value);
		SET_IRQ_CASE(UART_IRQ_SRC_ADET, UCR1, ADEN, UartUcr1Value);
		SET_IRQ_CASE(UART_IRQ_SRC_ACST, UCR3, ACIEN, UartUcr3Value);
		SET_IRQ_CASE(UART_IRQ_SRC_ESC, UCR2, ESCI, UartUcr2Value);
		SET_IRQ_CASE(UART_IRQ_SRC_IRINT, UCR4, ENIRI, UartUcr4Value);
		SET_IRQ_CASE(UART_IRQ_SRC_AIRINT, UCR3, AIRINTEN, UartUcr3Value);
		SET_IRQ_CASE(UART_IRQ_SRC_AWAKE, UCR3, AWAKEN, UartUcr3Value);
		SET_IRQ_CASE(UART_IRQ_SRC_FRAERR, UCR3, FRAERREN, UartUcr3Value);
		SET_IRQ_CASE(UART_IRQ_SRC_PARITYERR, UCR3, PARERREN, UartUcr3Value);
		SET_IRQ_CASE(UART_IRQ_SRC_RTSD, UCR1, RTSDEN, UartUcr1Value);
		SET_IRQ_CASE(UART_IRQ_SRC_RTSF, UCR2, RTSEN, UartUcr2Value);
		SET_IRQ_CASE(UART_IRQ_SRC_DTRF, UCR3, DTREN, UartUcr3Value);
		SET_IRQ_CASE(UART_IRQ_SRC_RIDELT, UCR3, RI, UartUcr3Value);
		SET_IRQ_CASE(UART_IRQ_SRC_DCDDELT, UCR3, DCD, UartUcr3Value);
		SET_IRQ_CASE(UART_IRQ_SRC_DTRD, UCR3, DTRDEN, UartUcr3Value);
		SET_IRQ_CASE(UART_IRQ_SRC_SAD, UMCR, SADEN, UartUmcrValue);

		default:
			PANIC("unhandled UART irq source");
	}
#undef SET_IRQ_CASE
}

/// Returns the cached irq state value
static inline bool UART_get_irq_state(UART_ID id, UART_IRQ_SOURCE irq)
{
#ifdef TEST
	if (irq >= UART_IRQ_SRC_COUNT) {
		PANIC("invalid input");
	}
#endif

	return (bool)BITFIELD32_GET(UART_IRQ_STATE[id], irq);
}

bitfield32 UART_get_irq_sources(UART_ID id)
{
	uintptr periph_base = UART_N_BASE[id];

	UartUsr1Value usr1 = UART_USR1_read(periph_base);
	UartUsr2Value usr2 = UART_USR2_read(periph_base);

	bitfield32 sources = 0;

#define SET_SRC(bit, status) \
	sources |= ((bitfield32)(((status) & UART_get_irq_state(id, bit)) << (bit)))

	SET_SRC(UART_IRQ_SRC_RRDY, UART_USR1_BF_get_RRDY(usr1));
	SET_SRC(UART_IRQ_SRC_IDLE, UART_USR2_BF_get_IDLE(usr2));
	SET_SRC(UART_IRQ_SRC_RDR, UART_USR2_BF_get_RDR(usr2));
	SET_SRC(UART_IRQ_SRC_RXDS, UART_USR1_BF_get_RXDS(usr1));
	SET_SRC(UART_IRQ_SRC_AGTIM, UART_USR1_BF_get_AGTIM(usr1));

	SET_SRC(UART_IRQ_SRC_TXFE, UART_USR2_BF_get_TXFE(usr2));
	SET_SRC(UART_IRQ_SRC_TRDY, UART_USR1_BF_get_TRDY(usr1));
	SET_SRC(UART_IRQ_SRC_TXDC, UART_USR2_BF_get_TXDC(usr2));

	SET_SRC(UART_IRQ_SRC_ORE, UART_USR2_BF_get_ORE(usr2));
	SET_SRC(UART_IRQ_SRC_BRCD, UART_USR2_BF_get_BRCD(usr2));
	SET_SRC(UART_IRQ_SRC_WAKE, UART_USR2_BF_get_WAKE(usr2));
	SET_SRC(UART_IRQ_SRC_ADET, UART_USR2_BF_get_ADET(usr2));
	SET_SRC(UART_IRQ_SRC_ACST, UART_USR2_BF_get_ACST(usr2));
	SET_SRC(UART_IRQ_SRC_ESC, UART_USR1_BF_get_ESCF(usr1));
	SET_SRC(UART_IRQ_SRC_IRINT, UART_USR2_BF_get_IRINT(usr2));
	SET_SRC(UART_IRQ_SRC_AIRINT, UART_USR1_BF_get_AIRINT(usr1));
	SET_SRC(UART_IRQ_SRC_AWAKE, UART_USR1_BF_get_AWAKE(usr1));
	SET_SRC(UART_IRQ_SRC_FRAERR, UART_USR1_BF_get_FRAERR(usr1));
	SET_SRC(UART_IRQ_SRC_PARITYERR, UART_USR1_BF_get_PARITYERR(usr1));
	SET_SRC(UART_IRQ_SRC_RTSD, UART_USR1_BF_get_RTSD(usr1));
	SET_SRC(UART_IRQ_SRC_RTSF, UART_USR2_BF_get_RTSF(usr2));
	SET_SRC(UART_IRQ_SRC_DTRF, UART_USR2_BF_get_DTRF(usr2));
	SET_SRC(UART_IRQ_SRC_RIDELT, UART_USR2_BF_get_RIDELT(usr2));
	SET_SRC(UART_IRQ_SRC_DCDDELT, UART_USR2_BF_get_DCDDELT(usr2));
	SET_SRC(UART_IRQ_SRC_DTRD, UART_USR1_BF_get_DTRD(usr1));
	SET_SRC(UART_IRQ_SRC_SAD, UART_USR1_BF_get_SAD(usr1));

#undef SET_SRC

	return sources;
}

// Handlers
typedef void (*uart_irq_handler_t)(UART_ID id);
void unhandled_irq_source(UART_ID) { PANIC("unhandled"); }

// UART_IRQ_SRC_RRDY: Received data interrupt handler (with the threashold set
// to 1) adds the received data to the rx buffer
static void handle_RRDY(UART_ID id)
{
	uintptr periph_base = UART_N_BASE[id];
#ifdef TEST
	if (UART_UTS_BF_get_RXEMPTY(UART_UTS_read(periph_base)))
		PANIC("RRDY irq arrived but no data was available");
#endif
	// Push all the uart fifo data into the driver ring buffer
	do {
		UartUrxdValue urxd = UART_URXD_read(periph_base);
		uint8 data = UART_URDX_BF_get_RX_DATA(urxd);

		bool non_overwrite = UART_rxbuf_push(id, data);
		if (!non_overwrite)
			PANIC("Uart rx buffer overwrite");	// TODO: better handling of
												// overwrites

	} while (!UART_UTS_BF_get_RXEMPTY(UART_UTS_read(periph_base)));
}

// UART_IRQ_SRC_TRDY: Tx hardware fifo reached less or the stablished value (4
// for UART_init), it tries to fill the buffer again with the data saved in the
// software driver tx buffer
static void handle_TRDY(UART_ID id)
{
	uintptr periph_base = UART_N_BASE[id];

#ifdef TEST
	if (!UART_USR1_BF_get_TRDY(UART_USR1_read(periph_base)))
		PANIC("TRDY irq arrived but irq bit was not on");
#endif

	uint8 data;
	bool txbuf_empty = false;
	while (!UART_UTS_BF_get_TXFULL(UART_UTS_read(periph_base))) {
		txbuf_empty = !UART_txbuf_pop(id, &data);

		if (txbuf_empty) {
			// disable the tx threashold irq, as there is
			// no more data available to send. It is enabled again when using
			// UART_putc
			UART_set_irq_state(id, UART_IRQ_SRC_TRDY, false);

			break;
		}

		// Send the data
		UART_UTXD_write(periph_base, data);
	}
}

static const uart_irq_handler_t UART_IRQ_SOURCE_HANDLERS[] = {
	[UART_IRQ_SRC_RRDY] = handle_RRDY,
	[UART_IRQ_SRC_RRDY + 1 ... UART_IRQ_SRC_TRDY - 1] = unhandled_irq_source,
	[UART_IRQ_SRC_TRDY] = handle_TRDY,
	[UART_IRQ_SRC_TRDY + 1 ... UART_IRQ_SRC_COUNT - 1] = unhandled_irq_source,
};

void UART_handle_irq(UART_ID id)
{
	bitfield32 source = UART_get_irq_sources(id);

	for (size_t i = UART_IRQ_SRC_START; i < UART_IRQ_SRC_COUNT; i++) {
		if (BITFIELD32_GET(source, i)) {
			UART_IRQ_SOURCE_HANDLERS[i](id);
		}
	}
}

/*
	------------
		INIT
	------------
*/

void UART_init_stage0(UART_ID id)
{
	UART_IRQ_STATE[id] = 0;

	UART_reset(id);

	// 17.2.12.1 7357
	uintptr periph_base = UART_N_BASE[id];

	UartUcr1Value ucr1 = {0};
	UART_UCR1_BF_set_UARTEN(&ucr1, true);
	UART_UCR1_BF_set_IDEN(&ucr1, false);
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
	UART_UCR3_BF_set_RXDSEN(&ucr3, false);
	UART_UCR3_BF_set_AWAKEN(&ucr3, false);

	UART_UCR3_write(periph_base, ucr3);

	UartUcr4Value ucr4 = {0};
	UART_UCR4_BF_set_CTSTL(&ucr4, 31);
	UART_UCR4_write(periph_base, ucr4);

	UartUfcrValue ufcr = {0};		  // 0000 1010 0000 0001
	UART_UFCR_BF_set_RXTL(&ufcr, 1);  // RX fifo threashold interrupt 1
	UART_UFCR_BF_set_TXTL(&ufcr, 4);  // TX fifo threashold interrupt 4
	UART_UFCR_BF_set_DCEDTE(&ufcr, false);
	UART_UFCR_BF_set_RFDIV(&ufcr, UART_UFCR_RFDIV_DIV_BY_2);

	UART_UFCR_write(periph_base, ufcr);

	UartUbirValue ubir = {0};
	UART_UBIR_BF_set_INC(&ubir, 0xF);
	UART_UBIR_write(periph_base, ubir);

	UartUbmrValue ubmr = {0};
	UART_UBMR_BF_set_MOD(&ubmr, 0x68);
	UART_UBMR_write(periph_base, ubmr);

	UartUmcrValue umcr = {0};
	UART_UMCR_write(periph_base, umcr);

	// Flush rx fifo
	UartUrxdValue urxd = UART_URXD_read(periph_base);
	while (!UART_UTS_BF_get_RXEMPTY(UART_UTS_read(periph_base))) {
		UART_URDX_BF_get_RX_DATA(urxd);
	}

	uint32 usr1_v = 0;
	for (size_t i = 0; i < 9; i++) {
		usr1_v |= (0b1 << USR1_IRQ_W1C_BITS[i]);
	}

	uint32 usr2_v = 0;
	for (size_t i = 0; i < 8; i++) {
		usr2_v |= (0b1 << USR2_IRQ_W2C_BITS[i]);
	}

	UART_USR1_write(periph_base, (UartUsr1Value){.val = usr1_v});
	UART_USR2_write(periph_base, (UartUsr2Value){.val = usr2_v});
}

void UART_init_stage1(UART_ID id)
{
	UART_set_irq_state(id, UART_IRQ_SRC_RRDY, true);
	UART_set_irq_state(id, UART_IRQ_SRC_TRDY, true);
}

/*
	------------
		put
	------------
*/

void UART_putc_sync(UART_ID id, const uint8 c)
{
	while (UART_tx_fifo_full(UART_N_BASE[id])) {
		for (size_t i = 0; i < 3000; i++) asm volatile("nop");
	}

	UART_UTXD_write(UART_N_BASE[id], c);
}

void UART_puts_sync(UART_ID id, const char *s)
{
	while (*s) UART_putc_sync(id, *s++);
}

void UART_putc(UART_ID id, const uint8 c)
{
	bool txbuf_full = !UART_txbuf_push(id, c);

	if (txbuf_full) PANIC("txbuf filled");	// TODO: handle better

	// If TRDY is not enabled enable it
	if (!UART_get_irq_state(id, UART_IRQ_SRC_TRDY))
		UART_set_irq_state(id, UART_IRQ_SRC_TRDY, true);
}

void UART_puts(const UART_ID id, const char *s)
{
	while (*s) UART_putc(id, *s++);
}