#pragma once

#include <lib/mem.h>
#include <lib/stdint.h>

#include "arm/mmu/mmu.h"

typedef struct {
    _Alignas(16) p_uintptr addr;
    _Alignas(16) size_t blocks;
    _Alignas(16) const char* tag;
    _Alignas(16) bool permanent;
} memblock;


void early_kalloc_init();


/// Allocates a kernel region and saves the memory blocks it allocated for later initialization of
/// other allocators
p_uintptr early_kalloc(size_t bytes, const char* tag, bool permanent);

/// Returns the pointer to the memblocks so later stage allocators like the page allocators can
/// update their structs and be coherent with the kernel memory blocks. The first time called it
/// reallocates all the structure to the end of the last allocated block and autoassigns itself as a
/// non permanent block. It is the other allocators job to free the early_kalloc block.
void early_kalloc_get_memblocks(memblock** memblocks, size_t* memblock_count);


#ifdef TEST
#    include <drivers/uart/uart.h>

#    include "kernel/devices/drivers.h"
#    include "lib/string.h"

inline void early_kalloc_debug()
{
    memblock* memblcks;
    size_t memblck_count;
    early_kalloc_get_memblocks(&memblcks, &memblck_count);

    for (size_t i = 0; i < memblck_count; i++) {
        char buf[200];

        UART_puts(&UART2_DRIVER, "memblock[");
        stdint_to_ascii((STDINT_UNION) {.uint64 = i}, STDINT_UINT64, buf, sizeof(buf),
                        STDINT_BASE_REPR_DEC);
        UART_puts(&UART2_DRIVER, buf);
        UART_puts(&UART2_DRIVER, "]\n\r");

        UART_puts(&UART2_DRIVER, "  addr      = ");
        stdint_to_ascii((STDINT_UNION) {.uint64 = memblcks[i].addr}, STDINT_UINT64, buf,
                        sizeof(buf), STDINT_BASE_REPR_HEX);
        UART_puts(&UART2_DRIVER, buf);
        UART_puts(&UART2_DRIVER, "\n\r");

        UART_puts(&UART2_DRIVER, "  blocks    = ");
        stdint_to_ascii((STDINT_UNION) {.uint64 = memblcks[i].blocks}, STDINT_UINT64, buf,
                        sizeof(buf), STDINT_BASE_REPR_DEC);
        UART_puts(&UART2_DRIVER, buf);
        UART_puts(&UART2_DRIVER, "\n\r");

        UART_puts(&UART2_DRIVER, "  size      = ");
        stdint_to_ascii((STDINT_UNION) {.uint64 = memblcks[i].blocks * MMU_GRANULARITY_4KB},
                        STDINT_UINT64, buf, sizeof(buf), STDINT_BASE_REPR_DEC);
        UART_puts(&UART2_DRIVER, buf);
        UART_puts(&UART2_DRIVER, " bytes\n\r");

        UART_puts(&UART2_DRIVER, "  permanent = ");
        UART_puts(&UART2_DRIVER, memblcks[i].permanent ? "true\n\r" : "false\n\r");

        UART_puts(&UART2_DRIVER, "  tag       = ");
        UART_puts(&UART2_DRIVER, memblcks[i].tag ? memblcks[i].tag : "(null)");
        UART_puts(&UART2_DRIVER, "\n\r\n\r");
    }
}
#else
static inline void early_kalloc_test()
{
}
#endif