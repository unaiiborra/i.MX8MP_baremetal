#include <drivers/uart/uart.h>
#include <kernel/devices/drivers.h>
#include <kernel/io/stdio.h>
#include <kernel/io/term.h>
#include <kernel/mm.h>
#include <lib/stdarg.h>
#include <lib/stdint.h>

#include "../devices/device_map.h"
#include "lib/stdmacros.h"


static term_handle stdout, stdwarn, stderr, stdpanic;

static term_handle* const STDIO_OUTPUTS[4] = {
    &stdout,
    &stdwarn,
    &stderr,
    &stdpanic,
};


static term_out_result early_putc(const char c)
{
    return uart_putc_early(c);
}


static term_out_result std_putc(const char c)
{
    return uart_putc(&UART2_DRIVER, c);
}


static term_out_result panic_putc(const char c)
{
    return uart_putc_sync(&UART2_DRIVER, c);
}


void io_early_init()
{
    uart_early_init(UART2_BASE);

    for (size_t i = 0; i < ARRAY_LEN(STDIO_OUTPUTS); i++)
        term_new(mm_as_kpa_ptr(STDIO_OUTPUTS[i]), early_putc);
}


void io_init()
{
    for (size_t i = 0; i < ARRAY_LEN(STDIO_OUTPUTS); i++) {
        term_delete(STDIO_OUTPUTS[i]);
    }

    // TODO: Change the std_putc for other outputs and add formatting like ansii colors etc.
    term_new(&stdout, std_putc);
    term_new(&stdwarn, std_putc);
    term_new(&stderr, std_putc);
    term_new(&stdpanic, panic_putc);
}


void io_flush(io_out io)
{
    term_flush(STDIO_OUTPUTS[io]);
}


void fkprintf(io_out io, const char* s, ...)
{
    va_list va;

    va_start(va, s);

    term_printf(STDIO_OUTPUTS[io], s, va);

    va_end(va);
}


void fkprint(io_out io, const char* s)
{
    term_prints(STDIO_OUTPUTS[io], s);
}
