/* kernel.c - main kernel compilation unit */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/heap.h>
#include <kernel/proc.h>
#include <kernel/scheduler.h>
#include <kernel/thread.h>

static struct thread_mutex lock;

static void kthread_1(__unused void *arg)
{
	while (1)
	{
		thread_mutex_acquire(&lock);
		kdprintf("B");
		thread_sleep(2000);
		thread_mutex_release(&lock);
		thread_sleep(200);
	}
}

static void kthread_2(__unused void *arg)
{
	while (1)
	{
		thread_mutex_acquire(&lock);
		kdprintf("C");
		thread_mutex_release(&lock);
		thread_sleep(50);
	}
}

noreturn kernel_main(__unused void *arg)
{
	kdprintf("Hello, kernel World!\n");

	thread_mutex_create(&lock);
	thread_create(PID_KERNEL, kthread_1, NULL);
	thread_create(PID_KERNEL, kthread_2, NULL);

	for (int i = 0; i < 100; i++)
	{
		kdprintf("A");
		/* Test to see if paging_propagate_changes works. */
		kalloc(HEAP_NORMAL, HEAP_NO_ALIGN, 4096);
		thread_sleep(1000);
	}
	/* Trigger a page fault to test panic IPI. */
	*((char*)0) = 0;
	while(1);
}
