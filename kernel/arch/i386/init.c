/* init.c - generic x86 initialization */
#include <kernel/cdefs.h>
#include <kernel/init.h>
#include <arch/cpu.h>
#include <arch/gdt.h>
#include <arch/idt.h>
#include <arch/init.h>
#include <arch/interrupts.h>

noreturn generic_x86_init(void)
{
	init_cpu();
	init_gdt();
	init_idt();
	init_isr();
	load_idt();
	yaos2_initialized = 1;
	kernel_main();
}
