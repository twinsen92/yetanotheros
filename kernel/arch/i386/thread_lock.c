/* thread_mutex.c - x86 implementation of thread_mutex */
#include <kernel/cdefs.h>
#include <kernel/cpu.h>
#include <kernel/debug.h>
#include <kernel/thread.h>
#include <arch/cpu.h>
#include <arch/scheduler.h>
#include <arch/thread.h>

/* TODO: Perhaps use some lighter alternative instead of preempt_disable/push_no_interrupts */

void _thread_mutex_create(struct thread_mutex *mutex, const char *file, unsigned int line)
{
	cpu_spinlock_create(&(mutex->spinlock), "thread mutex spinlock");
	thread_cond_create(&(mutex->wait_cond));
	mutex->locked = false;
	mutex->tid = TID_INVALID;
#ifdef KERNEL_DEBUG
	mutex->creation_file = file;
	mutex->creation_line = line;
	debug_clear_call_stack(&(mutex->lock_call_stack));
#endif
}

static inline bool unsafe_mutex_held(struct thread_mutex *mutex)
{
	struct x86_thread *thread;

	thread = get_current_thread();

	if (thread == NULL)
		kpanic("unsafe_mutex_held(): no thread running");

	return mutex->locked && (mutex->tid == thread->noarch.tid);
}

static inline void unsafe_mutex_acquire(struct thread_mutex *mutex)
{
	struct x86_thread *thread;

	thread = get_current_thread();

	if (thread == NULL)
		kpanic("thread_mutex_acquire(): no thread running");

	if (unsafe_mutex_held(mutex))
		kpanic("thread_mutex_acquire(): on held lock");

	/* Check in a loop if we have locked the mutex with an atomic exchange. If not, go into blocked
	   state. Another thread calling release on this mutex will notify and wake us up. */
	while (mutex->locked)
		sched_thread_wait(&(mutex->wait_cond), &(mutex->spinlock));

	mutex->locked = true;
	mutex->tid = thread->noarch.tid;

#ifdef KERNEL_DEBUG
	debug_fill_call_stack(&(mutex->lock_call_stack));
#endif
}

void thread_mutex_acquire(struct thread_mutex *mutex)
{
	/* Acquiring a spinlock no longer disables interrupts. To protect from weird behaviour if an
	   interrupt handler uses the same mutex, temporarily disable interrupts */
	cpu_spinlock_acquire(&(mutex->spinlock));
	push_no_interrupts();

	unsafe_mutex_acquire(mutex);

	pop_no_interrupts();
	cpu_spinlock_release(&(mutex->spinlock));
}

static inline void unsafe_mutex_release(struct thread_mutex *mutex)
{
	if (mutex->tid == TID_INVALID)
		kpanic("thread_mutex_release(): called on an unheld mutex");

	if (mutex->tid != TID_INVALID && !unsafe_mutex_held(mutex))
		kpanic("thread_mutex_release(): called on an unheld mutex");

	mutex->tid = TID_INVALID;

#ifdef KERNEL_DEBUG
	debug_clear_call_stack(&(mutex->lock_call_stack));
#endif

	/* Actually release the mutex. */
	mutex->locked = false;
}

void thread_mutex_release(struct thread_mutex *mutex)
{
	/* Acquiring a spinlock no longer disables interrupts. To protect from weird behaviour if an
	   interrupt handler uses the same mutex, temporarily disable interrupts */
	cpu_spinlock_acquire(&(mutex->spinlock));
	push_no_interrupts();

	unsafe_mutex_release(mutex);

	/* Notify a thread waiting on the internal conditon. */
	sched_thread_notify_one(&(mutex->wait_cond));

	pop_no_interrupts();
	cpu_spinlock_release(&(mutex->spinlock));
}

bool thread_mutex_held(struct thread_mutex *mutex)
{
	bool ret;

	/* Acquiring a spinlock no longer disables interrupts. To protect from weird behaviour if an
	   interrupt handler uses the same mutex, temporarily disable interrupts */
	cpu_spinlock_acquire(&(mutex->spinlock));
	push_no_interrupts();

	ret = unsafe_mutex_held(mutex);

	pop_no_interrupts();
	cpu_spinlock_release(&(mutex->spinlock));

	return ret;
}

void _thread_cond_create(struct thread_cond *cond, const char *file, unsigned int line)
{
	atomic_store(&(cond->num_waiting), 0);
#ifdef KERNEL_DEBUG
	cond->creation_file = file;
	cond->creation_line = line;
#endif
}

void thread_cond_wait(struct thread_cond *cond, struct thread_mutex *mutex)
{
	/* Acquiring a spinlock no longer disables interrupts. To protect from weird behaviour if an
	   interrupt handler uses the same mutex, temporarily disable interrupts */
	cpu_spinlock_acquire(&(mutex->spinlock));
	push_no_interrupts();

	if (!unsafe_mutex_held(mutex))
		kpanic("thread_cond_wait(): on unheld mutex");

	unsafe_mutex_release(mutex);

	/* Notify a thread waiting on the internal conditon. */
	sched_thread_notify_one(&(mutex->wait_cond));

	sched_thread_wait(cond, &(mutex->spinlock));

	unsafe_mutex_acquire(mutex);

	pop_no_interrupts();
	cpu_spinlock_release(&(mutex->spinlock));
}

void thread_cond_notify(struct thread_cond *cond)
{
	sched_thread_notify_one(cond);
}
