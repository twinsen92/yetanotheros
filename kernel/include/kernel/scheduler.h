/* kernel/scheduler.h - arch-independent scheduling interface */
#ifndef _KERNEL_SCHEDULER_H
#define _KERNEL_SCHEDULER_H

#include <kernel/proc.h>
#include <kernel/thread.h>

/* Gets the object of the thread currently running on the current CPU. */
struct thread *get_current_thread(void);

/* Get the process object from the scheduler. Returns NULL if the process could not be found.
   Acquires the scheduler lock if the process was found. */
struct proc *sheduler_get_proc(pid_t pid);

/* Put the process object back into the scheduler. Releases the scheduler lock if the process is not
   NULL. */
void sheduler_put_proc(struct proc *proc);

/* Insert a proc and its main thread into the scheduler, making it runnable. Transfers ownership
   of the proc object. */
pid_t schedule_proc(struct proc *proc, struct thread *thread);

/* Insert the thread into the scheduler, making it runnable. Transfers ownership of the thread
   object. */
tid_t schedule_thread(pid_t pid, struct thread *thread);

/* Create and schedule a thread in the kernel process. */
#define schedule_kernel_thread(entry, cookie, name) \
	schedule_thread(PID_KERNEL, kthread_create(entry, cookie, name))

/* Reset performance counters. */
void schedule_reset_perf_counters(void);

#endif
