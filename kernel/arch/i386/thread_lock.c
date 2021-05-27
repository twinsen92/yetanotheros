/* thread_mutex.c - x86 implementation of thread_mutex */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/interrupts.h>
#include <kernel/thread.h>
#include <arch/cpu.h>
#include <arch/scheduler.h>
#include <arch/thread.h>

#define asm_barrier() asm volatile ("" : : : "memory")

void thread_mutex_create(struct thread_mutex *mutex)
{
	atomic_store(&(mutex->locked), false);
	mutex->tid = TID_INVALID;
	thread_cond_create(&(mutex->wait_cond));
}

void thread_mutex_acquire(struct thread_mutex *mutex)
{
	struct x86_thread *thread;

	/* We don't want to get rescheduled when using get_current_thread() and modifying the mutex
	  object. */
	push_no_interrupts();

	thread = get_current_thread();

	if (thread == NULL)
		kpanic("thread_mutex_acquire(): no thread running");

	/* Check in a loop if we have locked the mutex with an atomic exchange. If not, go into blocked
	   state. Another thread calling release on this mutex will notify and wake us up. */
	while (atomic_exchange(&(mutex->locked), true))
		sched_thread_wait(&(mutex->wait_cond), NULL);

	asm_barrier();

	mutex->tid = thread->noarch.tid;

	pop_no_interrupts();
}

void thread_mutex_release(struct thread_mutex *mutex)
{
	/* We do not want to get rescheduled while accessing the current thread object and modifying
	   the mutex object. */
	push_no_interrupts();

	if (mutex->tid == TID_INVALID)
		kpanic("thread_mutex_release(): called on an unheld mutex");

	if (mutex->tid != TID_INVALID && !thread_mutex_held(mutex))
		kpanic("thread_mutex_release(): called on an unheld mutex");

	mutex->tid = TID_INVALID;

	asm_barrier();

	/* Actually release the mutex. */
	atomic_store(&(mutex->locked), false);

	/* Notify a thread waiting on the internal conditon. */
	sched_thread_notify_one(&(mutex->wait_cond));

	/* Continue with preemption enabled. */
	pop_no_interrupts();
}

bool thread_mutex_held(struct thread_mutex *mutex)
{
	bool ret;
	struct x86_thread *thread;

	push_no_interrupts();

	thread = get_current_thread();

	if (thread == NULL)
		kpanic("thread_mutex_held(): no thread running");

	ret = atomic_load(&(mutex->locked)) && mutex->tid == get_current_thread()->noarch.tid;

	pop_no_interrupts();

	return ret;
}

void thread_cond_create(struct thread_cond *cond)
{
	atomic_store(&(cond->num_waiting), 0);
}

void thread_cond_wait(struct thread_cond *cond, struct thread_mutex *mutex)
{
	sched_thread_wait(cond, mutex);
}

void thread_cond_notify(struct thread_cond *cond)
{
	sched_thread_notify_one(cond);
}