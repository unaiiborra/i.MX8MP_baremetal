#include <arm/mmu/mmu.h>
#include <drivers/uart/uart.h>

#include "../devices/device_map.h"
#include "kernel/init.h"
#include "kernel/io/term.h"
#include "kernel/mm.h"


static term_out_result early_out(char c)
{
    uart_putc_early(c);
    return TERM_OUT_RES_OK;
}


void kernel_early_init(void)
{
    uart_early_init(UART2_BASE);
    term_init_early(early_out);


    mm_early_init(); // returns to the kernel entry
}
