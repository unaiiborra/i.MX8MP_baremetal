#include <boot/panic.h>
#include <drivers/interrupts/gicv3/gicv3.h>
#include <drivers/interrupts/gicv3/gicv3_raw/gicv3_raw.h>
#include <lib/stdint.h>

#include "arm/exceptions/exceptions.h"
#include "drivers/interrupts/gicv3/gicv3_raw/gicd_ctlr.h"
#include "drivers/interrupts/gicv3/gicv3_raw/gicd_icfgr.h"
#include "drivers/interrupts/gicv3/gicv3_raw/gicd_ipriorityr.h"
#include "drivers/interrupts/gicv3/gicv3_raw/gicd_irouter.h"
#include "drivers/interrupts/gicv3/gicv3_raw/gicd_isenabler.h"
#include "drivers/interrupts/gicv3/gicv3_raw/gicr_waker.h"
#include "drivers/uart/uart.h"
#include "lib/string.h"

// Declared at gicv3_arm_interface.S
extern void _GICV3_ARM_ICC_SRE_EL1_write(uint64 v);
extern void _GICV3_ARM_ICC_PMR_EL1_write(uint64 v);
extern void _GICV3_ARM_ICC_IGRPEN1_EL1_write(uint64 v);
extern void _GICV3_ARM_ICC_EOIR1_EL1_write(uint64 v);
extern uint64 _GICV3_ARM_ICC_IAR1_EL1_read(void);

void GICV3_set_cpu_priority_threshold(uint8 threshold)
{
	_GICV3_ARM_ICC_PMR_EL1_write(threshold);
}

static inline void GICV3_arm_interface_enable(void)
{
	// Enable system register interface
	_GICV3_ARM_ICC_SRE_EL1_write(1);
	_GICV3_ARM_ICC_PMR_EL1_write(0xFF);
	_GICV3_ARM_ICC_IGRPEN1_EL1_write(1);
}

/// If irq number is not valid panics
static void GICV3_validate_spi_id(uint32 intid)
{
	GicdTyper typer = GICV3_GICD_TYPER_read();
	uint32 itlines = (uint32)GICV3_GICD_TYPER_BF_get_ITLinesNumber(typer);

	uint32 max_spi = (32 * (itlines + 1)) - 1;

	if (intid < 32 || intid > max_spi) PANIC("Invalid SPI INTID");
}

void GICV3_set_spi_group1ns(imx8mp_irq irq, bool v)
{
	// imx8mp irqs start from 32 as 0..31 in the gic are reserved
	uint32 intid = irq + 32;

	GICV3_validate_spi_id(intid);

	uint32 n = ((uint32)intid) / 32;
	uint32 bit = ((uint32)intid) % 32;

	GicdIgroupr igroupr = GICV3_GICD_IGROUPR_read(n);
	GICV3_GICD_IGROUPR_BF_set(&igroupr, bit, v);
	GICV3_GICD_IGROUPR_write(n, igroupr);
}

void GICV3_route_spi_to_cpu(imx8mp_irq irq, ARM_cpu_affinity affinity)
{
	uint32 intid = irq + 32;

	GICV3_validate_spi_id(intid);

	GicdIrouter r = {0};

	GICV3_GICD_IROUTER_BF_set_Interrupt_Routing_Mode(&r, false);

	GICV3_GICD_IROUTER_BF_set_Aff3(&r, affinity.aff3);
	GICV3_GICD_IROUTER_BF_set_Aff2(&r, affinity.aff2);
	GICV3_GICD_IROUTER_BF_set_Aff1(&r, affinity.aff1);
	GICV3_GICD_IROUTER_BF_set_Aff0(&r, affinity.aff0);

	GICV3_GICD_IROUTER_write(intid, r);
}

void GICV3_route_spi_to_self(imx8mp_irq irq)
{
	GICV3_route_spi_to_cpu(irq, ARM_get_cpu_affinity());
}

void GICV3_enable_spi(imx8mp_irq irq)
{
	uint32 intid = irq + 32;

	GICV3_validate_spi_id(intid);

	uint32 n = ((uint32)intid) / 32;
	uint32 bit = ((uint32)intid) % 32;

	GICV3_GICD_ISENABLER_set_bit(n, bit);
}

void GICV3_set_priority(imx8mp_irq irq, uint8 priority)
{
	uint32 intid = irq + 32;

	GICV3_validate_spi_id(intid);

	uint32 n = ((uint32)intid) / 4;
	uint32 byte_idx = ((uint32)intid) % 4;

	GicdIpriority r = GICV3_GICD_IPRIORITYR_read(n);
	GICV3_GICD_IPRIORITYR_BF_set(&r, byte_idx, priority);
	GICV3_GICD_IPRIORITYR_write(n, r);
}

void GICV3_set_edge_triggered(imx8mp_irq irq)
{
	uint32 intid = irq + 32;

	GICV3_validate_spi_id(intid);

	uint32 n = ((uint32)intid) / 16;
	uint32 slot = ((uint32)intid) % 16;

	GicdIcfgr r = GICV3_GICD_ICFGR_read(n);
	GICV3_GICD_ICFGR_BF_set(&r, slot, 0b10);
	GICV3_GICD_ICFGR_write(n, r);
}

void GICV3_set_level_sensitive(imx8mp_irq irq)
{
	uint32 intid = irq + 32;

	GICV3_validate_spi_id(intid);

	uint32 n = ((uint32)intid) / 16;
	uint32 slot = ((uint32)intid) % 16;

	GicdIcfgr r = GICV3_GICD_ICFGR_read(n);
	GICV3_GICD_ICFGR_BF_set(&r, slot, 0b00);
	GICV3_GICD_ICFGR_write(n, r);
}

void GICV3_wake_redistributor(size_t n)
{
	GicrWaker w = GICV3_GICR_WAKER_read(n);
	GICV3_GICR_WAKER_BF_set_ProcessorSleep(&w, false);
	GICV3_GICR_WAKER_write(n, w);

	bool asleep = true;

	while (asleep) {
		for (size_t i = 0; i < 5000; i++) asm volatile("nop");

		GicrWaker r = GICV3_GICR_WAKER_read(n);
		asleep = GICV3_GICR_WAKER_BF_get_ChildrenAsleep(r);
	}
}

void GICV3_init_distributor(void)
{
	GICV3_GICD_CTLR_write((GicdCtlr){.val = 0});  // disable
	asm volatile("dsb sy");

	// TODO: clean pending

	GicdCtlr ctlr = {0};
	GICV3_GICD_CTLR_BF_set_EnableGrp1NS(&ctlr, true);
	GICV3_GICD_CTLR_write(ctlr);

	asm volatile("dsb sy");
}

void GICV3_init_cpu(size_t cpu)
{
	GICV3_wake_redistributor(cpu);
	GICV3_arm_interface_enable();
}

void GICV3_init_irq(imx8mp_irq irq, uint8 priority, gicv3_irq_trigger trigger,
					ARM_cpu_affinity cpu)
{
	GICV3_set_spi_group1ns(irq, true);

	switch (trigger) {
		case GICV3_LEVEL_SENSITIVE:
			GICV3_set_level_sensitive(irq);
			break;

		case GICV3_EDGE_TRIGGERED:
			GICV3_set_edge_triggered(irq);
			break;

		default:
			PANIC("Invalid enum");
			break;
	}

	GICV3_set_priority(irq, priority);

	GICV3_route_spi_to_cpu(irq, cpu);

	GICV3_enable_spi(irq);
}

void uart_irq_init()
{
	imx8mp_irq uart_irq = IMX8MP_IRQ_UART2;

	// Non secure group
	GICV3_set_spi_group1ns(uart_irq, true);

	GICV3_set_level_sensitive(uart_irq);

	GICV3_set_priority(uart_irq, 0x80);

	GICV3_route_spi_to_self(uart_irq);

	GICV3_enable_spi(uart_irq);
}

uint64 GICV3_get_intid_el1()
{
	return _GICV3_ARM_ICC_IAR1_EL1_read() & 0xFFFFFF;
}

imx8mp_irq GICV3_imx8mp_irq_from_intid(uint64 intid)
{
#if TEST
	if (ARM_get_exception_level() != 1)
		PANIC("Reading EL1 source while not being in EL1");

	if (intid <= 32 || intid > IMX8MP_IRQ_SIZE + 32) {
		char buf[200];
		stdint_to_ascii((STDINT_UNION){.uint64 = intid}, STDINT_UINT64, buf,
						200, STDINT_BASE_REPR_DEC);
		UART_puts(UART_ID_2, "\r\nUnhandled irq number:\t");
		UART_puts(UART_ID_2, buf);

		PANIC("UNHANDLED IRQ");
	}
#endif

	return (imx8mp_irq)(intid - 32);
}

void GICV3_ack_intid_el1(uint64 irq_token)
{
	_GICV3_ARM_ICC_EOIR1_EL1_write(irq_token);
}