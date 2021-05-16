/* proc.c - x86 process list manager */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/proc.h>
#include <kernel/queue.h>
#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <arch/memlayout.h>
#include <arch/paging.h>
#include <arch/proc.h>
#include <arch/thread.h>

static struct spinlock processes_lock;
static struct x86_proc kernel_process;
static struct x86_proc_list processes;
static atomic_uint next_pid = 1;
static struct x86_proc *cursor;

/* Initializes the process list. */
void init_plist(void)
{
	spinlock_create(&processes_lock, "process list");

	kernel_process.pd = vm_map_rev_walk(kernel_pd, true);
	LIST_INIT(&(kernel_process.threads));
	kernel_process.noarch.pid = PID_KERNEL;
	kernel_process.noarch.state = PROC_NEW;

	LIST_INIT(&processes);
	LIST_INSERT_HEAD(&processes, &kernel_process, pointers);
	cursor = LIST_FIRST(&processes);
}

/* Acquires the process list lock. */
void plist_acquire(void)
{
	spinlock_acquire(&processes_lock);
}

/* Releases the process list lock. */
void plist_release(void)
{
	spinlock_release(&processes_lock);
}

/* Checks if the lock is held by the current CPU. */
bool plist_held(void)
{
	return spinlock_held(&processes_lock);
}

/* Gets a process with the given pid. Returns NULL if not found. Requires the process table lock. */
struct x86_proc *proc_get(unsigned int pid)
{
	struct x86_proc *proc;

	kassert(spinlock_held(&processes_lock));

	LIST_FOREACH(proc, &processes, pointers)
		if (proc->noarch.pid == pid)
			return proc;

	return NULL;
}

/* Get the kernel process object. Does not require the process table lock. */
struct x86_proc *proc_get_kernel_proc(void)
{
	return &kernel_process;
}

/* Look for the first READY thread in a READY process. Requires the process table lock. */
struct x86_thread *proc_get_first_ready_thread(void)
{
	struct x86_proc *proc;
	struct x86_thread *thread;

	kassert(spinlock_held(&processes_lock));

	LIST_FOREACH(proc, &processes, pointers)
		if (proc->noarch.state == PROC_READY)
			LIST_FOREACH(thread, &(proc->threads), pointers)
				if (thread->noarch.state == THREAD_READY)
					return thread;

	return NULL;
}

/* Adds the given thread to the given process and sets both to READY. Requires the process table
   lock. */
void proc_add_thread(struct x86_proc *proc, struct x86_thread *thread)
{
	kassert(spinlock_held(&processes_lock));

	LIST_INSERT_HEAD(&(proc->threads), thread, pointers);

	if (thread->noarch.state == THREAD_NEW)
		thread->noarch.state = THREAD_READY;

	if (proc->noarch.state == PROC_NEW)
		proc->noarch.state = PROC_READY;
}

/* Process list traversal. Resets the cursor to first process. Requires the process table lock. */
struct x86_proc *proc_cursor_reset(void)
{
	kassert(spinlock_held(&processes_lock));
	return cursor = LIST_FIRST(&processes);
}

/* Process list traversal. Moves cursor to next process. Requires the process table lock. */
struct x86_proc *proc_cursor_next(void)
{
	kassert(spinlock_held(&processes_lock));
	return cursor = LIST_NEXT(cursor, pointers);
}
