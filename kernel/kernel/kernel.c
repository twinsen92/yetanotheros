/* kernel.c - main kernel compilation unit */
#include <kernel/cdefs.h>
#include <kernel/debug.h>

noreturn kernel_main(void *arg)
{
	kdprintf("Hello, kernel World!\n");
	*((char*)arg) = 0;
	while(1);
}
