#pragma once

#include <kernel/irq/interrupts.h>
#include <drivers/interrupts/gicv3/gicv3.h>


void
kernel_handle_irq(irq_id irq);
