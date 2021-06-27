/* proc.c - x86 process structures and process list manager */
#include <kernel/cdefs.h>
#include <kernel/cpu.h>
#include <kernel/debug.h>
#include <kernel/heap.h>
#include <kernel/proc.h>
#include <kernel/scheduler.h>
#include <kernel/thread.h>
#include <kernel/ticks.h>
#include <arch/cpu.h>
#include <arch/memlayout.h>
#include <arch/paging.h>
#include <arch/proc.h>
#include <arch/thread.h>
#include <arch/cpu/selectors.h>

/* Scheduler lock. This applies to all structures used by the scheduler on all CPUs. */
static struct cpu_spinlock global_scheduler_lock;

/* Scheduler checkpoint used to ensure all CPUs enter the scheduler at the same time. */
static struct cpu_checkpoint scheduler_checkpoint;

/* Process list. */
static struct proc kernel_process;
static struct arch_proc kernel_arch_process;
static struct proc_list processes;
__unused static atomic_uint next_pid = 1;

/* Thread queue. */
static STAILQ_HEAD(thread_queue, thread) queue;

/* Adds the given thread to the given process and sets both to READY. Requires the process table
   lock. */
static void thread_new_insert(struct proc *proc, struct thread *thread)
{
	kassert(cpu_spinlock_held(&global_scheduler_lock));

	LIST_INSERT_HEAD(&(proc->threads), thread, lptrs);

	thread->state = THREAD_READY;
	proc->state = PROC_READY;

	if (thread->state == THREAD_READY)
		STAILQ_INSERT_TAIL(&queue, thread, sqptrs);
}

/* Removes the given thread, marking it THREAD_TRUNCATE. Also checks if the parent process can be
   marked as PROC_DEFUNCT. */
static void thread_destroy(struct thread *thread)
{
	struct proc *proc;

	kassert(cpu_spinlock_held(&global_scheduler_lock));
	kassert(thread->state == THREAD_EXITED);

	/* Free the thread's resources. */
	/* TODO: We might have to switch to process' CR3 here. */
	proc = thread->parent;
	LIST_REMOVE(thread, lptrs);
	/* thread->stack also belongs to us */
	kfree(thread->arch->stack);
	kfree(thread);

	if (LIST_EMPTY(&(proc->threads)))
	{
		/* We do not want to destroy the kernel process! */
		kassert(proc != &kernel_process);
		proc->state = PROC_DEFUNCT;
	}

	/* TODO: Free process' resources. */
}

/* arch/scheduler.h interface */

/* Initializes the global scheduler data and locks. */
void init_global_scheduler(void)
{
	cpu_spinlock_create(&global_scheduler_lock, "global scheduler");

	cpu_checkpoint_create(&scheduler_checkpoint);

	kernel_arch_process.pd = vm_map_rev_walk(kernel_pd, true);
	kernel_process.arch = &kernel_arch_process;
	LIST_INIT(&(kernel_process.threads));
	kernel_process.pid = PID_KERNEL;
	kernel_process.state = PROC_NEW;

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

static inline void store_interrupts(struct thread *thread, struct x86_cpu *cpu)
{
	thread->arch->int_enabled = cpu->int_enabled;
	thread->arch->cli_stack = cpu->cli_stack;
}

static inline void restore_interrupts(struct thread *thread, struct x86_cpu *cpu)
{
	cpu->int_enabled = thread->arch->int_enabled;
	cpu->cli_stack = thread->arch->cli_stack;

	if (cpu->int_enabled && cpu->cli_stack == 0)
		cpu_force_sti();
	else
		cpu_force_cli();
}

/* Enters the scheduler. This creates a scheduler thread in the kernel process for the current
   CPU and starts the scheduler loop. This is final. */
noreturn enter_scheduler(void)
{
	struct x86_cpu *cpu;
	struct proc *proc;
	struct thread *thread;

	cpu = cpu_current();

	/* Create the scheduler thread. */
	cpu_spinlock_acquire(&global_scheduler_lock);
	cpu->scheduler_thread.arch = &(cpu->scheduler_arch_thread);
	x86_thread_construct_empty(&(cpu->scheduler_thread), &kernel_process, "scheduler thread",
		KERNEL_CODE_SELECTOR, KERNEL_DATA_SELECTOR);
	cpu->scheduler = &(cpu->scheduler_thread);
	thread_new_insert(&kernel_process, cpu->scheduler);
	/* The scheduler thread should not be scheduled! */
	cpu->scheduler->state = THREAD_SCHEDULER;
	cpu_spinlock_release(&global_scheduler_lock);

	/* Set the scheduler fields in the CPU. */
	cpu->thread = NULL;
	thread = NULL;
	proc = NULL;

	/* Wait for all other CPUs to enter the scheduler loop. */
	cpu_checkpoint_enter(&scheduler_checkpoint);

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
		if (thread->state == THREAD_EXITED)
		{
			/* Thread has exited. Destroy it and try getting a new thread from the queue. */
			thread_destroy(thread);
			goto _scheduler_continue;
		}

		if (thread->state == THREAD_SLEEPING)
		{
			if (thread->sleep_until < ticks_get())
			{
				/* We can now run it. */
				thread->state = THREAD_READY;
				thread->sleep_since = 0;
				thread->sleep_until = 0;
			}
			else
			{
				/* Still sleeping. Put it back in the queue. */
				STAILQ_INSERT_TAIL(&queue, thread, sqptrs);
				goto _scheduler_continue;
			}
		}

		/* Try running the thread. */
		if (thread->state == THREAD_READY)
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
			cpu_set_cr3(proc->arch->pd);

			thread->state = THREAD_RUNNING;

			x86_thread_switch(cpu->scheduler->arch, thread->arch);

			/* TODO: Switch back to kernel CR3. */
			cpu_set_cr3(kernel_arch_process.pd);

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

static void reschedule(void)
{
	struct x86_cpu *cpu = cpu_current();
	struct thread *thread = cpu->thread;

	/* We should not hold more than the global scheduler lock. Otherwise we can run into issues
	   if the thread ends up on a different CPU. */
	if (cpu->preempt_disabled > 1)
		kpanic("reschedule(): preemption was disabled");

	if (thread == NULL)
		kpanic("reschedule(): no thread is running");
	if (!cpu_spinlock_held(&global_scheduler_lock))
		kpanic("reschedule(): process list lock not held");
	if (thread->state == THREAD_RUNNING)
		kpanic("reschedule(): bad thread state");

	/* Put the thread back on the scheduler's queue. */
	if (thread->state != THREAD_BLOCKED)
		STAILQ_INSERT_TAIL(&queue, thread, sqptrs);

	/* Do the switch. The scheduler loop takes care of interrupts stack. */
	x86_thread_switch(thread->arch, cpu->scheduler->arch);
}

/* Make the current thread wait on the given condition. A spinlock is unlocked and then relocked. */
void sched_thread_wait(struct thread_cond *cond, struct cpu_spinlock *spinlock)
{
	struct thread *thread;

	kassert(cond && spinlock);

	 /* We release the spinlock while holding the global scheduler lock. This way we can be sure no
		other thread notifies the condition between spinlock release and reschedule() */
	if (spinlock != &global_scheduler_lock)
	{
		cpu_spinlock_acquire(&global_scheduler_lock);
		cpu_spinlock_release(spinlock);
	}

	thread = get_current_thread();
	atomic_fetch_add(&(cond->num_waiting), 1);
	thread->state = THREAD_BLOCKED;
	thread->cond = cond;
	reschedule();
	atomic_fetch_sub(&(cond->num_waiting), 1);
	thread->cond = NULL;


	/* We wait on the spinlock outside of the section where we hold the global scheduler lock. This
	   way we can avoid getting stuck on acquire and never being able to schedule the thread
	   currently holding it. */
	if (spinlock != &global_scheduler_lock)
	{
		cpu_spinlock_release(&global_scheduler_lock);
		cpu_spinlock_acquire(spinlock);
	}
}

/* Notify a thread waiting on the given mutex that it is unlocked. Puts the thread at the beginning
   of the thread queue. */
void sched_thread_notify_one(struct thread_cond *cond)
{
	struct proc *proc;
	struct thread *thread;

	cpu_spinlock_acquire(&global_scheduler_lock);

	LIST_FOREACH(proc, &processes, pointers)
	{
		LIST_FOREACH(thread, &(proc->threads), lptrs)
		{
			if (thread->cond == cond && thread->state == THREAD_BLOCKED)
			{
				thread->state = THREAD_READY;
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

	struct thread *thread = get_current_thread();
	thread->entry(thread->cookie);
	thread_exit();
}

/* Creates a kernel thread in the process identified by pid. */
tid_t thread_create(pid_t pid, void (*entry)(void *), void *cookie, const char *name)
{
	tid_t tid;
	struct proc *proc;
	struct thread *thread;
	vaddr_t stack;

	if (pid != PID_KERNEL)
		kpanic("thread_create(): pid != PID_KERNEL unsupported");

	proc = &kernel_process;
	stack = kalloc(HEAP_NORMAL, 16, 4096);
	thread = kalloc(HEAP_NORMAL, HEAP_NO_ALIGN, sizeof(struct thread));
	thread->arch = kalloc(HEAP_NORMAL, HEAP_NO_ALIGN, sizeof(struct arch_thread));
	x86_thread_construct_kthread(thread, proc, name, stack, 4096, &thread_entry, entry,
		cookie, true);

	/* We get the ID before putting the thread into the scheduler structures. The thread might exit
	   and be destroyed before we even return from this function. */
	tid = thread->tid;

	cpu_spinlock_acquire(&global_scheduler_lock);
	thread_new_insert(proc, thread);
	cpu_spinlock_release(&global_scheduler_lock);

	return tid;
}

/* Gets the object of the thread currently running on the current CPU. */
struct thread *get_current_thread(void)
{
	struct x86_cpu *cpu;
	struct thread *thread;

	/* Don't want to get rescheduled between cpu_current and cpu->thread access. */
	preempt_disable();
	cpu = cpu_current();
	thread = cpu->thread;
	preempt_enable();

	return thread;
}

/* Forces the current thread to be rescheduled. */
void thread_yield(void)
{
	cpu_spinlock_acquire(&global_scheduler_lock);
	get_current_thread()->state = THREAD_READY;
	reschedule();
	cpu_spinlock_release(&global_scheduler_lock);
}

/* Exits the current thread. */
noreturn thread_exit(void)
{
	cpu_spinlock_acquire(&global_scheduler_lock);
	get_current_thread()->state = THREAD_EXITED;
	reschedule();
	kpanic("thread_exit(): thread returned");
}

/* Puts the current thread to sleep for a given number of milliseconds. */
void thread_sleep(unsigned int milliseconds)
{
	struct thread *thread;
	ticks_t cur;

	cpu_spinlock_acquire(&global_scheduler_lock);
	thread = get_current_thread();
	thread->state = THREAD_SLEEPING;
	cur = ticks_get();
	thread->sleep_since = cur;
	thread->sleep_until = cur + (milliseconds * TICKS_PER_MILLISECOND);
	reschedule();
	cpu_spinlock_release(&global_scheduler_lock);
}
