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
#include <arch/thread.h>

noreturn generic_x86_init(void)
{
	if (!mpt_scan())
		kpanic("Did not find MP tables!");

	/* Initialize shared subsystems. */
	init_gdt();
	init_paging();
	init_kernel_heap(&(vm_map[VM_DYNAMIC_REGION]));

	/* Test the kernel heap. TODO: Remove. */
	kalloc(HEAP_NORMAL, 16, 20);
	vaddr_t v2 = kalloc(HEAP_NORMAL, 1, 12345);
	vaddr_t v3 = kalloc(HEAP_NORMAL, 1, 12345);
	kfree(v2);
	kfree(v3);

	init_idt();
	init_isr();
	pic_disable();
	init_global_scheduler();

	/* Initialize the first LAPIC. */
	load_idt();
	init_lapic();

	/* The kernel has been initialized now. */
	yaos2_initialized = 1;

	cpu_set_active(true);
	thread_create(PID_KERNEL, kernel_main, NULL);
	enter_scheduler();
}
