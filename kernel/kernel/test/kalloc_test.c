/* kalloc_test.c - a stress test for kalloc() to find potential race conditions and deadlocks */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/heap.h>
#include <kernel/proc.h>
#include <kernel/scheduler.h>
#include <kernel/thread.h>

static struct thread_mutex lock;

static void kthread_1(__unused void *arg)
{
	int *v;

	v = kalloc(HEAP_NORMAL, HEAP_NO_ALIGN, 4096);

	while (1)
	{
		thread_mutex_acquire(&lock);
		kdprintf("B");
		*v = 0x12345678;
		thread_sleep(2000);
		kassert(*v == 0x12345678);
		thread_mutex_release(&lock);
		thread_sleep(200);
		v = kalloc(HEAP_NORMAL, HEAP_NO_ALIGN, 4096);
	}
}

static void kthread_2(__unused void *arg)
{
	vaddr_t v;

	v = kalloc(HEAP_NORMAL, HEAP_NO_ALIGN, 4096);

	while (1)
	{
		thread_mutex_acquire(&lock);
		kdprintf("C");
		kfree(v);
		thread_mutex_release(&lock);
		thread_sleep(50);
		v = kalloc(HEAP_NORMAL, HEAP_NO_ALIGN, 4096);
	}
}

static void kthread_3(__unused void *arg)
{
	while (1)
	{
		thread_mutex_acquire(&lock);
		kdprintf("D");
		thread_mutex_release(&lock);
		thread_sleep(10000);
		thread_create(PID_KERNEL, kthread_3, NULL);
	}
}

noreturn kalloc_test_main(void)
{
	thread_mutex_create(&lock);
	thread_create(PID_KERNEL, kthread_1, NULL);
	thread_create(PID_KERNEL, kthread_2, NULL);
	thread_create(PID_KERNEL, kthread_3, NULL);

	for (int i = 0; i < 100; i++)
	{
		kdprintf("A");
		/* Test to see if paging_propagate_changes works. */
		kalloc(HEAP_NORMAL, HEAP_NO_ALIGN, 4096);
		thread_sleep(1000);
	}

	while (1);
}
