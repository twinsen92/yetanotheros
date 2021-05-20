/* thread_lock.c - x86 implementation of thread_lock */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/interrupts.h>
#include <kernel/thread.h>
#include <arch/cpu.h>
#include <arch/scheduler.h>
#include <arch/thread.h>

#define asm_barrier() asm volatile ("" : : : "memory")

void thread_lock_create(struct thread_lock *lock)
{
	atomic_store(&(lock->locked), false);
	lock->tid = TID_INVALID;
}

void thread_lock_acquire(struct thread_lock *lock)
{
	/* We acquire the global scheduler lock. This prevents situations where thread_notify might
	   happen between the exchange and thread_wait_lock(). It doesn't make much of a difference in
	   terms of performance because thread_wait_lock() would have had to acquire the lock to put the
	   thread in THREAD_BLOCKED state anyway. This also disables interrupts/preemption, of course. */
	sched_global_acquire();

	/* Check in a loop if we have locked the lock with an atomic exchange. If not, go to sleep and
	   reschedule. The thread will be awakened with thread_notify by the current holder. */
	while (atomic_exchange(&(lock->locked), true))
		thread_wait_lock(lock);

	asm_barrier();

	lock->tid = get_current_thread()->noarch.tid;

	sched_global_release();
}

void thread_lock_release(struct thread_lock *lock)
{
	/* We do not want to get rescheduled while accessing the current thread object and modifying
	   the lock object. */
	push_no_interrupts();

	if (lock->tid == TID_INVALID)
		kpanic("thread_lock_release(): called on an unheld lock");

	if (lock->tid != TID_INVALID && !thread_lock_held(lock))
		kpanic("thread_lock_release(): called on an unheld lock");

	lock->tid = TID_INVALID;

	asm_barrier();

	/* Actually release the lock. */
	atomic_store(&(lock->locked), false);

	/* Continue with preemption enabled. */
	pop_no_interrupts();

	/* Lazily wake up another thread, which could be waiting at the lock. We do not really care that
	   it happens in preemptible part. If this leads to multiple threads being woken up to acquire
	   the lock, then so be it. */
	thread_notify(lock);
}

bool thread_lock_held(struct thread_lock *lock)
{
	push_no_interrupts();
	bool ret = atomic_load(&(lock->locked)) && lock->tid == get_current_thread()->noarch.tid;
	pop_no_interrupts();
	return ret;
}
