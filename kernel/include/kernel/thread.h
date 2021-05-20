/* thread.h - arch-independent threading structure */
#ifndef _KERNEL_THREAD_H
#define _KERNEL_THREAD_H

#include <kernel/cdefs.h>
#include <kernel/ticks.h>

#define KERNEL_TID_IDLE 0
#define TID_INVALID 0xffffffff

#define THREAD_NEW			0 /* Thread has just been created. */
#define THREAD_RUNNING		1 /* Thread is currently running. */
#define THREAD_SCHEDULER	2 /* Thread is the scheduler thread. */
#define THREAD_READY		3 /* Thread is ready to be scheduled. */
#define THREAD_BLOCKED		4 /* Thread is waiting on a lock. */
#define THREAD_SLEEPING		5 /* Thread is sleeping. */
#define THREAD_EXITED		6 /* Thread has exited. */

typedef unsigned int tid_t;
struct thread_lock;

struct thread
{
	tid_t tid; /* ID of the thread */
	int state; /* Current thread state. */
	ticks_t sleep_since; /* Sleep start tick, if state == THREAD_SLEEPING. */
	ticks_t sleep_until; /* Sleep end tick, if state == THREAD_SLEEPING. */
	struct thread_lock *lock; /* Lock this thread is waiting on, if state == THREAD_BLOCKED. */
	char name[32]; /* Name of the thread. */
};

/* thread_lock - a preemtible lock that puts waiting threads into THREAD_BLOCKED state */

struct thread_lock
{
	/* Is the lock acquired? */
	atomic_bool locked;

	/* Holder of the lock. */
	tid_t tid;
};

void thread_lock_create(struct thread_lock *lock);
void thread_lock_acquire(struct thread_lock *lock);
void thread_lock_release(struct thread_lock *lock);
bool thread_lock_held(struct thread_lock *lock);

#endif
