/* init.c - generic x86 initialization */
#include <kernel/cdefs.h>
#include <kernel/init.h>
#include <arch/gdt.h>

noreturn generic_x86_init(void)
{
	init_gdt();
	kernel_main();
}
