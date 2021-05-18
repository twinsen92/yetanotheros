/* scheduler.h - arch-independent scheduling interface */
#ifndef _KERNEL_SCHEDULER_H
#define _KERNEL_SCHEDULER_H

#include <kernel/cdefs.h>

/* Creates a thread in the process identified by pid. */
void thread_create(unsigned int pid, void (*entry)(void *), void *cookie);

/* Forces the current thread to be rescheduled. */
void thread_yield(void);

/* Exits the current thread. */
noreturn thread_exit(void);

/* Puts the current thread to sleep for a given number of milliseconds. */
void thread_sleep(unsigned int milliseconds);

#endif
