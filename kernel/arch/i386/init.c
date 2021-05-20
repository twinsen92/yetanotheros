/* init.c - generic x86 initialization */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/heap.h>
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

static noreturn cpu_scheduler_entry(void);

noreturn generic_x86_init(void)
{
	if (!mpt_scan())
		kpanic("Did not find MP tables!");

	/* Enumerate stuff in MP tables. */
	mpt_enum_cpus();
	mpt_enum_ioapics();

	/* Some early CPU initialization. */
	init_gdt();
	cpu_set_boot_cpu();

	/* Initialize the registry for low-level interrupt handlers. */
	init_idt();
	init_isr();

	/* Initialize shared subsystems. */
	init_paging();
	init_kernel_heap(&(vm_map[VM_DYNAMIC_REGION]));

	/* Test the kernel heap. TODO: Remove. */
	kalloc(HEAP_NORMAL, 16, 20);
	vaddr_t v2 = kalloc(HEAP_NORMAL, 1, 12345);
	vaddr_t v3 = kalloc(HEAP_NORMAL, 1, 12345);
	kfree(v2);
	kfree(v3);

	pic_disable();
	init_global_scheduler();

	/* Initialize the first LAPIC. */
	load_idt();
	init_lapic();

	/* Init SMP stuff. */
	init_smp();

	/* The kernel has been initialized now. */
	yaos2_initialized = 1;

	thread_create(PID_KERNEL, kernel_main, NULL);

	start_aps();

	cpu_scheduler_entry();
}

noreturn cpu_entry(void)
{
	init_gdt();
	load_idt();
	init_lapic();
	cpu_scheduler_entry();
}

static noreturn cpu_scheduler_entry(void)
{
	cpu_set_active(true);
	enter_scheduler();
}
