/* kernel.c - main kernel compilation unit */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/heap.h>
#include <kernel/scheduler.h>

noreturn kernel_main(void *arg)
{
	kdprintf("Hello, kernel World!\n");
	for (int i = 0; i < 100; i++)
	{
		kdprintf("A");
		/* Test to see if paging_propagate_changes works. */
		kalloc(HEAP_NORMAL, HEAP_NO_ALIGN, 4096);
		thread_sleep(1000);
	}
	while(1);
}
