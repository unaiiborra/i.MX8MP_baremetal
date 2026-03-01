#include <arm/exceptions/exceptions.h>
#include <arm/tfa/smccc.h>
#include <drivers/arm_generic_timer/arm_generic_timer.h>
#include <drivers/interrupts/gicv3/gicv3.h>
#include <drivers/tmu/tmu.h>
#include <kernel/init.h>
#include <kernel/lib/kvec.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/stdint.h>
#include <lib/stdmacros.h>
#include <lib/string.h>

#include "arm/cpu.h"
#include "kernel/io/stdio.h"
#include "mm/mm_info.h"
#include "mm/phys/page_allocator.h"
#include "mm/virt/vmalloc.h"


static
void
debug_allocators(bool mmu_dump)
{
	if (mmu_dump)
		kprint("\n\r=== MMU ===\n\r");

	kprint("\n\r=== PAGE ALLOCATOR ===\n\r");
	page_allocator_debug();
	vmalloc_debug_free();
	vmalloc_debug_reserved();
}

// Main function of the kernel, called by the bootloader (/boot/boot.S)
_Noreturn void
kernel_entry()
{
	size_t coreid = ARM_get_cpu_affinity().aff0;

	if (coreid == 0) {
		if (!mm_kernel_is_relocated())
			kernel_early_init();
		else
			kernel_init();
	}

	__attribute((unused)) mm_ksections y = MM_KSECTIONS;


	kprint("\n\rSTART\n\r");

#define N    100

	debug_allocators(true);

	size_t i;
	void *raw_kmap_ptrs[N];
	void *raw_dynamic_ptrs[N];
	void *cache_ptrs[N];
	void *kmalloc_ptrs[N];

	for (i = 0; i < N; i++)
		raw_kmap_ptrs[i] = raw_kmalloc(1, "KMAP TEST", &RAW_KMALLOC_KMAP_CFG);

	for (i = 0; i < N; i++)
		raw_dynamic_ptrs[i] =
			raw_kmalloc(i + 1, "DYNAMIC TEST", &RAW_KMALLOC_DYNAMIC_CFG);

	for (i = 0; i < N; i++)
		cache_ptrs[i] = cache_malloc(CACHE_32);

	for (i = 0; i < N; i++)
		kmalloc_ptrs[i] = kmalloc(i * 8);


	debug_allocators(false);
	kprint("\n\rALLOCATIONS ENDED\n\r");


	for (i = 0; i < N; i++) {
		raw_kfree(raw_kmap_ptrs[i]);
		raw_kfree(raw_dynamic_ptrs[i]);
		cache_free(CACHE_32, cache_ptrs[i]);
		kfree(kmalloc_ptrs[i]);
	}


	debug_allocators(false);
	kprint("\n\rFREES ENDED\n\r");


	loop asm volatile ("wfi");
} /* kernel_entry */
