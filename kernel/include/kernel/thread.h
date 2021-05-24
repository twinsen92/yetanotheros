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
struct thread_mutex;

struct thread
{
	tid_t tid; /* ID of the thread */
	int state; /* Current thread state. */
	ticks_t sleep_since; /* Sleep start tick, if state == THREAD_SLEEPING. */
	ticks_t sleep_until; /* Sleep end tick, if state == THREAD_SLEEPING. */
	struct thread_cond *cond; /* Condition this thread is waiting on, if state == THREAD_BLOCKED. */
	char name[32]; /* Name of the thread. */
};

/* thread_cond - a preemtible condition that puts waiting threads into THREAD_BLOCKED state */
struct thread_cond
{
	atomic_int num_waiting;
};

/* thread_mutex - a preemtible mutex that puts waiting threads into THREAD_BLOCKED state */
struct thread_mutex
{
	/* Is the mutex acquired? */
	atomic_bool locked;

	struct thread_cond wait_cond;

	/* Holder of the mutex. */
	tid_t tid;
};

void thread_mutex_create(struct thread_mutex *mutex);
void thread_mutex_acquire(struct thread_mutex *mutex);
void thread_mutex_release(struct thread_mutex *mutex);
bool thread_mutex_held(struct thread_mutex *mutex);

void thread_cond_create(struct thread_cond *cond);
void thread_cond_wait(struct thread_cond *cond, struct thread_mutex *mutex);
void thread_cond_notify(struct thread_cond *cond);

#endif
