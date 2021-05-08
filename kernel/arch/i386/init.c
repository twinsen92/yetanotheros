/* init.c - generic x86 initialization */
#include <kernel/init.h>

__attribute__((__noreturn__))
void generic_x86_init(void)
{
	kernel_main();
}
