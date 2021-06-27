/* kernel/thread.h - arch-independent threading structure */
#ifndef _KERNEL_THREAD_H
#define _KERNEL_THREAD_H

#include <kernel/cdefs.h>
#include <kernel/cpu.h>
#include <kernel/debug.h>
#include <kernel/queue.h>
#include <kernel/ticks.h>

#define TID_INVALID 0

#define THREAD_NEW			0 /* Thread has just been created. */
#define THREAD_RUNNING		1 /* Thread is currently running. */
#define THREAD_SCHEDULER	2 /* Thread is the scheduler thread. */
#define THREAD_READY		3 /* Thread is ready to be scheduled. */
#define THREAD_BLOCKED		4 /* Thread is waiting on a lock. */
#define THREAD_SLEEPING		5 /* Thread is sleeping. */
#define THREAD_EXITED		6 /* Thread has exited. */

typedef unsigned int tid_t;

struct proc; /* Can't include proc.h due to it including thread.h */
struct arch_thread;

struct thread
{
	tid_t tid; /* ID of the thread */
	int state; /* Current thread state. */
	ticks_t sleep_since; /* Sleep start tick, if state == THREAD_SLEEPING. */
	ticks_t sleep_until; /* Sleep end tick, if state == THREAD_SLEEPING. */
	struct thread_cond *cond; /* Condition this thread is waiting on, if state == THREAD_BLOCKED. */
	char name[32]; /* Name of the thread. */

	void (*entry)(void *); /* Entry point the scheduler will call. */
	void *cookie; /* The scheduler will pass this cookie to the entry point. */

	struct proc *parent; /* Parent process */

	struct arch_thread *arch; /* Arch-dependent structure. */

	LIST_ENTRY(thread) lptrs;
	STAILQ_ENTRY(thread) sqptrs;
};

LIST_HEAD(thread_list, thread);

/* thread_cond - a preemtible condition that puts waiting threads into THREAD_BLOCKED state */
struct thread_cond
{
	atomic_int num_waiting;

#ifdef KERNEL_DEBUG
	const char *creation_file; /* Source file in which the condition was created. */
	unsigned int creation_line; /* Source file line in which the condition was created. */
#endif
};

/* thread_mutex - a preemtible mutex that puts waiting threads into THREAD_BLOCKED state */
struct thread_mutex
{
	struct cpu_spinlock spinlock;
	struct thread_cond wait_cond;

	bool locked; /* Is the mutex acquired? */

	/* Holder of the mutex. */
	tid_t tid;

#ifdef KERNEL_DEBUG
	const char *creation_file; /* Source file in which the condition was created. */
	unsigned int creation_line; /* Source file line in which the condition was created. */

	struct debug_call_stack lock_call_stack;
#endif
};

void _thread_mutex_create(struct thread_mutex *mutex, const char *file, unsigned int line);
#define thread_mutex_create(mutex) _thread_mutex_create(mutex, __FILE__, __LINE__)
void thread_mutex_acquire(struct thread_mutex *mutex);
void thread_mutex_release(struct thread_mutex *mutex);
bool thread_mutex_held(struct thread_mutex *mutex);

void _thread_cond_create(struct thread_cond *cond, const char *file, unsigned int line);
#define thread_cond_create(cond) _thread_cond_create(cond, __FILE__, __LINE__)
void thread_cond_wait(struct thread_cond *cond, struct thread_mutex *mutex);
void thread_cond_notify(struct thread_cond *cond);

#endif
