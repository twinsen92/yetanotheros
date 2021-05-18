/* kernel.c - main kernel compilation unit */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/scheduler.h>

noreturn kernel_main(void *arg)
{
	kdprintf("Hello, kernel World!\n");
	for (int i = 0; i < 100; i++)
	{
		kdprintf("A");
		thread_sleep(1000);
	}
	while(1);
}
