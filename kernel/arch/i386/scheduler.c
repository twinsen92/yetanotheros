/* proc.c - x86 process structures and process list manager */
#include <kernel/cdefs.h>
#include <kernel/cpu_spinlock.h>
#include <kernel/debug.h>
#include <kernel/heap.h>
#include <kernel/interrupts.h>
#include <kernel/proc.h>
#include <kernel/scheduler.h>
#include <kernel/thread.h>
#include <kernel/ticks.h>
#include <arch/cpu.h>
#include <arch/memlayout.h>
#include <arch/paging.h>
#include <arch/proc.h>
#include <arch/selectors.h>
#include <arch/thread.h>

/* Scheduler lock. This applies to all structures used by the scheduler on all CPUs. */
static struct cpu_spinlock global_scheduler_lock;

/* Process list. */
static struct x86_proc kernel_process;
static struct x86_proc_list processes;
static atomic_uint next_pid = 1;

/* Thread queue. */
static STAILQ_HEAD(x86_thread_queue, x86_thread) queue;

/* Adds the given thread to the given process and sets both to READY. Requires the process table
   lock. */
static void thread_new_insert(struct x86_proc *proc, struct x86_thread *thread)
{
	kassert(cpu_spinlock_held(&global_scheduler_lock));

	LIST_INSERT_HEAD(&(proc->threads), thread, lptrs);

	thread->noarch.state = THREAD_READY;
	proc->noarch.state = PROC_READY;

	if (thread->noarch.state == THREAD_READY)
		STAILQ_INSERT_TAIL(&queue, thread, sqptrs);
}

/* Removes the given thread, marking it THREAD_TRUNCATE. Also checks if the parent process can be
   marked as PROC_DEFUNCT. */
static void thread_destroy(struct x86_thread *thread)
{
	struct x86_proc *proc;

	kassert(cpu_spinlock_held(&global_scheduler_lock));
	kassert(thread->noarch.state == THREAD_EXITED);

	/* Free the thread's resources. */
	/* TODO: We might have to switch to process' CR3 here. */
	proc = thread->parent;
	LIST_REMOVE(thread, lptrs);
	/* thread->stack belongs to us */
	kfree(thread->stack);
	x86_thread_free(thread);

	if (LIST_EMPTY(&(proc->threads)))
	{
		/* We do not want to destroy the kernel process! */
		kassert(proc != &kernel_process);
		proc->noarch.state = PROC_DEFUNCT;
	}

	/* TODO: Free process' resources. */
}

/* arch/scheduler.h interface */

/* Initializes the global scheduler data and locks. */
void init_global_scheduler(void)
{
	cpu_spinlock_create(&global_scheduler_lock, "global scheduler");

	kernel_process.pd = vm_map_rev_walk(kernel_pd, true);
	LIST_INIT(&(kernel_process.threads));
	kernel_process.noarch.pid = PID_KERNEL;
	kernel_process.noarch.state = PROC_NEW;

	LIST_INIT(&processes);
	LIST_INSERT_HEAD(&processes, &kernel_process, pointers);

	STAILQ_INIT(&queue);
}

/* Acquire the global scheduler lock. Doing this will prevent any core from scheduling new threads
   or modifying thread's properties. */
void sched_global_acquire(void)
{
	cpu_spinlock_acquire(&global_scheduler_lock);
}

/* Undoes sched_global_acquire(). */
void sched_global_release(void)
{
	cpu_spinlock_release(&global_scheduler_lock);
}

static inline void store_interrupts(struct x86_thread *thread, struct x86_cpu *cpu)
{
	/* Thread switches should not happen with interrupts enabled. */
	kassert(cpu->cli_stack > 0);

	thread->int_enabled = cpu->int_enabled;
	thread->cli_stack = cpu->cli_stack;
}

static inline void restore_interrupts(struct x86_thread *thread, struct x86_cpu *cpu)
{
	/* Thread switches should not happen with interrupts enabled. */
	kassert(thread->cli_stack > 0);

	cpu->int_enabled = thread->int_enabled;
	cpu->cli_stack = thread->cli_stack;
}

/* Enters the scheduler. This creates a scheduler thread in the kernel process for the current
   CPU and starts the scheduler loop. This is final. */
noreturn enter_scheduler(void)
{
	struct x86_cpu *cpu;
	struct x86_proc *proc;
	struct x86_thread *thread;

	cpu = cpu_current();

	/* Create the scheduler thread. */
	cpu_spinlock_acquire(&global_scheduler_lock);
	cpu->scheduler = x86_thread_allocate_empty(&kernel_process, "scheduler thread",
		KERNEL_CODE_SELECTOR, KERNEL_DATA_SELECTOR);
	thread_new_insert(&kernel_process, cpu->scheduler);
	/* The scheduler thread should not be scheduled! */
	cpu->scheduler->noarch.state = THREAD_SCHEDULER;
	cpu_spinlock_release(&global_scheduler_lock);

	/* Set the scheduler fields in the CPU. */
	cpu->thread = NULL;
	thread = NULL;
	proc = NULL;

	while (true)
	{
		/* Enable interrupts on this CPU. */
		if ((cpu_get_eflags() & EFLAGS_IF) == 0)
			cpu_set_interrupts_with_cpu(cpu, true);

		cpu_spinlock_acquire(&global_scheduler_lock);

		/* Pop a thread from the queue. */
		thread = STAILQ_FIRST(&queue);

		if (!thread)
			goto _scheduler_continue;

		STAILQ_REMOVE_HEAD(&queue, sqptrs);

		/* Do some thread queue management. */
		if (thread->noarch.state == THREAD_EXITED)
		{
			/* Thread has exited. Destroy it and try getting a new thread from the queue. */
			thread_destroy(thread);
			goto _scheduler_continue;
		}

		if (thread->noarch.state == THREAD_SLEEPING)
		{
			if (thread->noarch.sleep_until < ticks_get())
			{
				/* We can now run it. */
				thread->noarch.state = THREAD_READY;
				thread->noarch.sleep_since = 0;
				thread->noarch.sleep_until = 0;
			}
			else
			{
				/* Still sleeping. Put it back in the queue. */
				STAILQ_INSERT_TAIL(&queue, thread, sqptrs);
				goto _scheduler_continue;
			}
		}

		/* Try running the thread. */
		if (thread->noarch.state == THREAD_READY)
		{
			proc = thread->parent;

			/* Store the original state of the interrupts in the scheduler thread. */
			store_interrupts(cpu->scheduler, cpu);

			/* Switch to chosen thread. It is the thread's job to release the process list lock and
			   reaquire it before switching back to scheduler. */
			cpu->thread = thread;

			/* Restore the thread's state of the interrupts. */
			restore_interrupts(cpu->thread, cpu);

			/* TODO: Reload TSS as well. */
			cpu_set_cr3(proc->pd);

			thread->noarch.state = THREAD_RUNNING;

			x86_thread_switch(cpu->scheduler, thread);

			/* TODO: Switch back to kernel CR3. */
			cpu_set_cr3(kernel_process.pd);

			/* Store the thread's state of interrupts. */
			store_interrupts(cpu->thread, cpu);

			/* Thread is done running for now. It should have changed its state before coming
			   back. */
			cpu->thread = NULL;

			/* Restore the original state of the interrupts from the scheduler thread. */
			restore_interrupts(cpu->scheduler, cpu);
		}
_scheduler_continue:
		cpu_spinlock_release(&global_scheduler_lock);
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
	struct x86_thread *thread = get_current_thread();

	if(!cpu_spinlock_held(&global_scheduler_lock))
		kpanic("reschedule(): process list lock not held");
	if(thread->noarch.state == THREAD_RUNNING)
		kpanic("reschedule(): bad thread state");
	if(cpu_get_eflags() & EFLAGS_IF)
		kpanic("reschedule(): interrupts enabled");

	/* Put the thread back on the scheduler's queue. */
	if (thread->noarch.state != THREAD_BLOCKED)
		STAILQ_INSERT_TAIL(&queue, thread, sqptrs);

	/* Do the switch. The scheduler loop takes care of interrupts stack. */
	x86_thread_switch(thread, cpu_current()->scheduler);
}

/* Make the current thread wait on the given lock. */
void thread_wait_lock(struct thread_lock *lock)
{
	struct x86_thread *thread;

	/* We expect the caller to hold the global scheduler lock. */
	if(!cpu_spinlock_held(&global_scheduler_lock))
		kpanic("thread_wait_lock(): global lock not held");

	thread = get_current_thread();
	thread->noarch.state = THREAD_BLOCKED;
	thread->noarch.lock = lock;
	reschedule();
	thread->noarch.lock = NULL;
}

/* Notify a thread waiting on the given lock that it is unlocked. Puts the thread at the beginning
   of the thread queue. */
void thread_notify(struct thread_lock *lock)
{
	struct x86_proc *proc;
	struct x86_thread *thread;

	cpu_spinlock_acquire(&global_scheduler_lock);

	LIST_FOREACH(proc, &processes, pointers)
	{
		LIST_FOREACH(thread, &(proc->threads), lptrs)
		{
			if (thread->noarch.lock == lock)
			{
				thread->noarch.state = THREAD_READY;
				thread->noarch.lock = NULL;
				STAILQ_INSERT_HEAD(&queue, thread, sqptrs);
			}
		}
	}

	cpu_spinlock_release(&global_scheduler_lock);
}

/* kernel/scheduler.h */

static void thread_entry(void)
{
	/* We enter with the global scheduler lock. We have to release it for other schedulers to work. */
	cpu_spinlock_release(&global_scheduler_lock);

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

	proc = &kernel_process;
	stack = kalloc(HEAP_NORMAL, 16, 4096);
	thread = x86_thread_allocate(proc, "some thread", KERNEL_CODE_SELECTOR, KERNEL_DATA_SELECTOR,
		stack, 4096, &thread_entry, entry, cookie);

	/* New threads start holding the global scheduler lock. */
	thread->int_enabled = true; /* The thread, by default, has interrupts enabled. */
	thread->cli_stack = 1;

	cpu_spinlock_acquire(&global_scheduler_lock);
	thread_new_insert(proc, thread);
	cpu_spinlock_release(&global_scheduler_lock);
}

/* Forces the current thread to be rescheduled. */
void thread_yield(void)
{
	cpu_spinlock_acquire(&global_scheduler_lock);
	get_current_thread()->noarch.state = THREAD_READY;
	reschedule();
	cpu_spinlock_release(&global_scheduler_lock);
}

/* Exits the current thread. */
noreturn thread_exit(void)
{
	cpu_spinlock_acquire(&global_scheduler_lock);
	get_current_thread()->noarch.state = THREAD_EXITED;
	reschedule();
	kpanic("thread_exit(): thread returned");
}

/* Puts the current thread to sleep for a given number of milliseconds. */
void thread_sleep(unsigned int milliseconds)
{
	struct x86_thread *thread;
	ticks_t cur;

	cpu_spinlock_acquire(&global_scheduler_lock);

	thread = get_current_thread();
	thread->noarch.state = THREAD_SLEEPING;
	cur = ticks_get();
	thread->noarch.sleep_since = cur;
	thread->noarch.sleep_until = cur + (milliseconds * (TICKS_PER_SECOND / 1000));
	reschedule();
	cpu_spinlock_release(&global_scheduler_lock);
}
