/* kernel/scheduler.h - arch-independent scheduling interface */
#ifndef _KERNEL_SCHEDULER_H
#define _KERNEL_SCHEDULER_H

#include <kernel/cdefs.h>
#include <kernel/proc.h>
#include <kernel/thread.h>

/* Creates a kernel thread in the process identified by pid. */
tid_t thread_create(pid_t pid, void (*entry)(void *), void *cookie, const char *name);

/* Gets the object of the thread currently running on the current CPU. */
struct thread *get_current_thread(void);

/* Forces the current thread to be rescheduled. */
void thread_yield(void);

/* Exits the current thread. */
noreturn thread_exit(void);

/* Puts the current thread to sleep for a given number of milliseconds. */
void thread_sleep(unsigned int milliseconds);

#endif
