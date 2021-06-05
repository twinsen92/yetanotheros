/* init.c - generic x86 initialization */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/init.h>
#include <kernel/proc.h>
#include <kernel/scheduler.h>
#include <arch/cpu.h>
#include <arch/heap.h>
#include <arch/init.h>
#include <arch/interrupts.h>
#include <arch/memlayout.h>
#include <arch/mpt.h>
#include <arch/pic.h>
#include <arch/serial.h>
#include <arch/scheduler.h>
#include <arch/thread.h>
#include <arch/cpu/apic.h>
#include <arch/cpu/gdt.h>
#include <arch/cpu/idt.h>
#include <arch/cpu/paging.h>
#include <arch/cpu/smp.h>

uint32_t yaos2_initialized;

noreturn early_kernel_main(__unused void *cookie)
{
	/* This needs to be called from a thread, because it uses thread locks. */
	debug_redirect_to_serial(serial_get_com1());

	/* Dump the contents of MP tables to the serial port. */
	mpt_dump();

	/* Continue on to the normal kernel main. */
	kernel_main();
}

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

	/* Initialize critical shared subsystems. */
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
	init_serial();

	thread_create(PID_KERNEL, early_kernel_main, NULL, "kernel_main");

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
