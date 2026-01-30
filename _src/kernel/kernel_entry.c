#include <arm/exceptions/exceptions.h>
#include <arm/tfa/smccc.h>
#include <boot/panic.h>
#include <drivers/arm_generic_timer/arm_generic_timer.h>
#include <drivers/interrupts/gicv3/gicv3.h>
#include <drivers/tmu/tmu.h>
#include <drivers/uart/uart.h>
#include <kernel/init.h>
#include <lib/memcpy.h>
#include <lib/stdint.h>
#include <lib/stdmacros.h>
#include <lib/string.h>

#include "../arm/mmu/mmu_pd.h"
#include "arm/cpu.h"
#include "arm/mmu/mmu.h"
#include "kernel/devices/drivers.h"
#include "kernel/mm/mm.h"
#include "lib/math.h"
#include "lib/unit/mem.h"
#include "mm/init/early_kalloc.h"
#include "mm/phys/page.h"
#include "mm/phys/page_allocator.h"
#include "mm/phys/tests.h"


// Main function of the kernel, called by the bootloader (/boot/boot.S)
_Noreturn void kernel_entry()
{
    ARM_cpu_affinity aff = ARM_get_cpu_affinity();

    if (aff.aff0 == 0) {
        kernel_init();
        uart_puts(&UART2_DRIVER, "\x1B[2J\x1B[H"); // clear screen

        mm_early_init();
        mm_init();

        uart_puts(&UART2_DRIVER, "DONE");
    }


    loop;
}