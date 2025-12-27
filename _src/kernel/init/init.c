#include <arm/exceptions/exceptions.h>
#include <boot/panic.h>
#include <drivers/interrupts/gicv3/gicv3.h>
#include <drivers/uart/uart.h>
#include <kernel/devices/drivers.h>
#include <kernel/init.h>
#include <lib/stdint.h>

extern kernel_initcall_t __kernel_init_stage0_start[];
extern kernel_initcall_t __kernel_init_stage0_end[];

extern kernel_initcall_t __kernel_init_stage1_start[];
extern kernel_initcall_t __kernel_init_stage1_end[];

extern kernel_initcall_t __kernel_init_stage2_start[];
extern kernel_initcall_t __kernel_init_stage2_end[];

extern void rust_kernel_initcalls_stage0(void);
extern void rust_kernel_initcalls_stage1(void);
extern void rust_kernel_initcalls_stage2(void);

KERNEL_INITCALL(rust_kernel_initcalls_stage0, KERNEL_INITCALL_STAGE0);
KERNEL_INITCALL(rust_kernel_initcalls_stage1, KERNEL_INITCALL_STAGE1);
KERNEL_INITCALL(rust_kernel_initcalls_stage2, KERNEL_INITCALL_STAGE2);

void kernel_init(void)
{
	// Stage 0 (pre irq initialization)
	for (kernel_initcall_t *fn = __kernel_init_stage0_start;
		 fn < __kernel_init_stage0_end; fn++) {
		(*fn)();
	}

	ARM_exceptions_set_status((ARM_exception_status){
		.fiq = true,
		.irq = true,
		.serror = true,
		.debug = true,
	});

	GICV3_init_distributor(&GIC_DRIVER);
	GICV3_init_cpu(&GIC_DRIVER, ARM_get_cpu_affinity().aff0);

	for (kernel_initcall_t *fn = __kernel_init_stage1_start;
		 fn < __kernel_init_stage1_end; fn++) {
		(*fn)();
	}

	for (kernel_initcall_t *fn = __kernel_init_stage2_start;
		 fn < __kernel_init_stage2_end; fn++) {
		(*fn)();
	}
}
