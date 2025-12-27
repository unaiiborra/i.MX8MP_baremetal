#include <drivers/uart/uart.h>
#include <kernel/devices/drivers.h>

#include "device_map.h"
#include "kernel/devices/device.h"

// UART1
uart_state uart1_state;
const driver_handle UART1_DRIVER = {
	.base = UART1_BASE,
	.state = &uart1_state,
};

// UART2
uart_state uart2_state;
const driver_handle UART2_DRIVER = {
	.base = UART2_BASE,
	.state = &uart2_state,
};

// UART3
uart_state uart3_state;
const driver_handle UART3_DRIVER = {
	.base = UART3_BASE,
	.state = &uart3_state,
};

// UART4
uart_state uart4_state;
const driver_handle UART4_DRIVER = {
	.base = UART4_BASE,
	.state = &uart4_state,
};

// GIC
const driver_handle GIC_DRIVER = {
	.base = GIC_BASE,
	.state = (void *)0,
};