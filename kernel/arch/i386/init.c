/* init.c - generic x86 initialization */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/init.h>
#include <kernel/proc.h>
#include <kernel/scheduler.h>
#include <arch/apic.h>
#include <arch/cpu.h>
#include <arch/gdt.h>
#include <arch/heap.h>
#include <arch/idt.h>
#include <arch/init.h>
#include <arch/interrupts.h>
#include <arch/memlayout.h>
#include <arch/mpt.h>
#include <arch/paging.h>
#include <arch/pic.h>
#include <arch/scheduler.h>
#include <arch/smp.h>
#include <arch/thread.h>

uint32_t yaos2_initialized;

noreturn generic_x86_init(void)
{
	if (!mpt_scan())
		kpanic("Did not find MP tables!");

	/* Enumerate stuff in MP tables. */
	mpt_enum_cpus();

	/* Some early CPU initialization. */
	init_gdt();
	cpu_set_boot_cpu();

	/* Initialize the registry for low-level interrupt handlers. */
	init_idt();
	init_isr();

	/* Initialize shared subsystems. */
	init_paging();
	init_kernel_heap(&(vm_map[VM_DYNAMIC_REGION]));

	pic_disable();
	init_global_scheduler();

	/* Initialize the first LAPIC. */
	load_idt();
	init_lapic();

	/* Init SMP stuff. */
	init_smp();

	/* Init x86-specific devices. */
	mpt_enum_ioapics();
	init_ioapics();

	thread_create(PID_KERNEL, kernel_main, NULL);

	/* The kernel has been initialized now. */
	yaos2_initialized = 1;
	/* This CPU is now "active". */
	cpu_set_active(true);

	start_aps();

	enter_scheduler();
}

noreturn cpu_entry(void)
{
	/* Initialize some basic x86 stuff. */
	init_gdt();
	load_idt();
	init_lapic();
	/* Set ourselves as active and enter scheduler. */
	cpu_set_active(true);
	enter_scheduler();
}
