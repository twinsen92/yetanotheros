/* init.c - generic x86 initialization */
#include <kernel/cdefs.h>
#include <kernel/init.h>

noreturn generic_x86_init(void)
{
	kernel_main();
}
