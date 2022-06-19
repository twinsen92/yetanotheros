/* kernel/thread.h - arch-independent threading structure */
#ifndef _KERNEL_THREAD_H
#define _KERNEL_THREAD_H

#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/cpu.h>
#include <kernel/debug.h>
#include <kernel/queue.h>
#include <kernel/ticks.h>

#include <user/yaos2/kernel/types.h>

#define TID_INVALID 0

enum thread_state
{
	/* Thread is the scheduler thread. */
	THREAD_SCHEDULER = -1,

	/* Thread has just been created. */
	THREAD_NEW = 0,

	/* Thread is ready to be scheduled. */
	THREAD_READY,

	/* Thread is currently running. */
	THREAD_RUNNING,

	/* Thread is waiting for child process. */
	THREAD_WAITING,

	/* Thread is waiting on a lock. */
	THREAD_BLOCKED,

	/* Thread is sleeping. */
	THREAD_SLEEPING,

	/* Thread has exited. */
	THREAD_EXITED,
};

typedef unsigned int tid_t;

struct proc; /* Can't include proc.h due to it including thread.h */
struct arch_thread;

struct thread
{
	/* Constant part. */

	tid_t tid; /* ID of the thread */
	char name[32]; /* Name of the thread. */
	struct proc *parent; /* Parent process */
	struct arch_thread *arch; /* Arch-dependent structure. */

	void (*entry)(void *); /* Entry point the scheduler will call. */
	void *cookie; /* The scheduler will pass this cookie to the entry point. */

	/* Dynamic part. Protected by global scheduler lock. */

	int state; /* Current thread state. */
	pid_t collected_pid;
	int collected_status;
	ticks_t sleep_since; /* Sleep start tick, if state == THREAD_SLEEPING. */
	ticks_t sleep_until; /* Sleep end tick, if state == THREAD_SLEEPING. */
	struct thread_cond *cond; /* Condition this thread is waiting on, if state == THREAD_BLOCKED. */
	uint sched_count;

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

/* Creates a kernel thread. */
struct thread *kthread_create(void (*entry)(void *), void *cookie, const char *name);

struct thread *uthread_create(uvaddr_t tentry, uvaddr_t stack, size_t stack_size,
	const char *name, struct proc *parent);

/* Frees a thread object. */
void thread_free(struct thread *thread);

/* Forces the current thread to be rescheduled. */
void thread_yield(void);

/* Exits the current thread. */
noreturn thread_exit(void);

/* Puts the current thread to sleep for a given number of milliseconds. */
void thread_sleep(unsigned int milliseconds);

#endif
