/* kernel.c - main kernel compilation unit */
#include <kernel/cdefs.h>
#include <kernel/debug.h>

noreturn kernel_main(void)
{
	kdprintf("Hello, kernel World!\n");
	while(1);
}
