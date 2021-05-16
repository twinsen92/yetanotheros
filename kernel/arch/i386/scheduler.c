/* proc.c - x86 process structures and process list manager */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/heap.h>
#include <kernel/interrupts.h>
#include <kernel/proc.h>
#include <kernel/scheduler.h>
#include <kernel/thread.h>
#include <arch/cpu.h>
#include <arch/memlayout.h>
#include <arch/paging.h>
#include <arch/proc.h>
#include <arch/selectors.h>
#include <arch/thread.h>

/* arch/scheduler.h interface */

/* Enters the scheduler. This creates a scheduler thread in the kernel process for the current
   CPU and starts the scheduler loop. This is final. */
noreturn enter_scheduler(void)
{
	struct x86_cpu *cpu;
	struct x86_proc *kernel_proc;
	struct x86_proc *proc;
	struct x86_thread *thread;

	cpu = cpu_current();

	/* Create the scheduler thread. */
	kernel_proc = proc_get_kernel_proc();
	plist_acquire();
	cpu->scheduler = x86_thread_allocate_empty(kernel_proc, "scheduler thread",
		KERNEL_CODE_SELECTOR, KERNEL_DATA_SELECTOR);
	proc_add_thread(kernel_proc, cpu->scheduler);
	/* The scheduler thread should not be scheduled! The kernel proc, however, should always be
	   schedule-able. */
	kernel_proc->noarch.state = PROC_READY;
	cpu->scheduler->noarch.state = THREAD_SCHEDULER;
	plist_release();

	/* Set the scheduler fields in the CPU. */
	cpu->thread = NULL;
	thread = NULL;
	proc = NULL;

	while (true)
	{
		// Enable interrupts on this processor.
		if ((cpu_get_eflags() & EFLAGS_IF) == 0)
			cpu_set_interrupts_with_cpu(cpu, true);

		// Loop over process table looking for process to run.
		plist_acquire();
		thread = proc_get_first_ready_thread();
		if (thread)
		{
			proc = thread->parent;

			/* Switch to chosen thread. It is the thread's job to release the process list lock and
			   reaquire it before switching back to scheduler. */
			cpu->thread = thread;

			/* TODO: Reload TSS as well. */
			cpu_set_cr3_with_cpu(cpu, proc->pd);

			/* We allow the kernel process to be scheduled on any number of CPUs. */
			if (proc != kernel_proc)
				proc->noarch.state = PROC_RUNNING;
			thread->noarch.state = THREAD_RUNNING;

			x86_thread_switch(cpu->scheduler, thread);

			/* TODO: Switch back to kernel CR3. */
			cpu_set_cr3_with_cpu(cpu, kernel_proc->pd);

			/* Thread is done running for now. It should have changed its state before coming
			   back. */
			cpu->thread = NULL;
		}
		plist_release();
	}
}

/* Gets the object of the thread currently running on the current CPU. */
struct x86_thread *get_current_thread(void)
{
	struct x86_cpu *cpu;
	struct x86_thread *thread;

	/* Don't want to get rescheduled between cpu_current and cpu->thread access. */
	push_no_interrupts();
	cpu = cpu_current();
	thread = cpu->thread;
	pop_no_interrupts();

	return thread;
}

static void reschedule(void)
{
	bool int_enabled;
	struct x86_thread *thread = get_current_thread();

	if(!plist_held())
		kpanic("reschedule(): process list lock not held");
	if(cpu_current()->cli_stack != 1) /* TODO: Review this. */
		kpanic("reschedule(): bad CLI stack");
	if(thread->noarch.state == THREAD_RUNNING)
		kpanic("reschedule(): bad thread state");
	/* Update the process state. We treat the kernel process specially. */
	if (thread->parent != proc_get_kernel_proc())
		thread->parent->noarch.state = PROC_READY;
	/* TODO: Check if process has any other threads running. */
	if(cpu_get_eflags() & EFLAGS_IF)
		kpanic("reschedule(): interrupts enabled");

	/* We pass the interrupts enabled flag to the CPU that picks up the thread. */
	int_enabled = cpu_current()->int_enabled;
	x86_thread_switch(thread, cpu_current()->scheduler);
	cpu_current()->int_enabled = int_enabled;
}

/* kernel/scheduler.h */

static void thread_entry()
{
	/* We enter with the process list lock. We have to release it for other schedulers to work. */
	plist_release();

	if((cpu_get_eflags() & EFLAGS_IF) == 0)
		kpanic("thread_entry(): interrupts not enabled");

	struct x86_thread *thread = get_current_thread();
	thread->entry(thread->cookie);
	thread_exit();
}

/* Creates a thread in the process identified by pid. */
void thread_create(unsigned int pid, void (*entry)(void *), void *cookie)
{
	struct x86_proc *proc;
	struct x86_thread *thread;
	vaddr_t stack;

	if (pid != PID_KERNEL)
		kpanic("thread_create(): pid != PID_KERNEL unsupported");

	proc = proc_get_kernel_proc();
	stack = kalloc(HEAP_NORMAL, 16, 4096);
	thread = x86_thread_allocate(proc, "some thread", KERNEL_CODE_SELECTOR, KERNEL_DATA_SELECTOR,
		stack, 4096, &thread_entry, entry, cookie);

	plist_acquire();
	proc_add_thread(proc, thread);
	plist_release();
}

/* Forces the current thread to be rescheduled. */
void thread_yield(void)
{
	plist_acquire();
	get_current_thread()->noarch.state = THREAD_READY;
	reschedule();
	plist_release();
}

/* Exits the current thread. */
noreturn thread_exit(void)
{
	plist_acquire();
	get_current_thread()->noarch.state = THREAD_EXITED;
	reschedule();
	kpanic("thread_exit(): thread returned");
}
