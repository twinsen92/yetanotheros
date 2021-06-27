/* arch/scheduler.h - x86 arch-dependent scheduler interface */
#ifndef ARCH_I386_SCHEDULER_H
#define ARCH_I386_SCHEDULER_H

#include <kernel/cdefs.h>
#include <kernel/cpu.h>
#include <kernel/thread.h>

/* Initializes the global scheduler data and locks. */
void init_global_scheduler(void);

/* Acquire the global scheduler lock. Doing this will prevent any core from scheduling new threads
   or modifying thread's properties. */
void sched_global_acquire(void);

/* Undoes sched_global_acquire(). */
void sched_global_release(void);

/* Enters the scheduler. This creates a scheduler thread in the kernel process for the current
   CPU and starts the scheduler loop. This is final. */
noreturn enter_scheduler(void);

/* Make the current thread wait on the given condition. A spinlock is unlocked and then relocked. */
void sched_thread_wait(struct thread_cond *cond, struct cpu_spinlock *spinlock);

/* Notify a thread waiting on the given cond that it is unlocked. Puts the thread at the beginning
   of the thread queue. */
void sched_thread_notify_one(struct thread_cond *cond);

#endif
