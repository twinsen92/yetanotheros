/* proc.h - x86 process structures and process list manager */
#ifndef ARCH_I386_PROC_H
#define ARCH_I386_PROC_H

#include <kernel/addr.h>
#include <kernel/queue.h>
#include <kernel/proc.h>
#include <kernel/thread.h>
#include <arch/paging_types.h>
#include <arch/thread.h>

struct x86_proc
{
	paddr_t pd; /* Page directory used by this process. */

	struct x86_thread_list threads; /* Thread list. */
	struct proc noarch; /* Arch-independent structure. */

	LIST_ENTRY(x86_proc) pointers;
};

LIST_HEAD(x86_proc_list, x86_proc);

/* Initializes the process list. */
void init_plist(void);

/* Acquires the process list lock. */
void plist_acquire(void);

/* Releases the process list lock. */
void plist_release(void);

/* Checks if the lock is held by the current CPU. */
bool plist_held(void);

/* Gets a process with the given pid. Returns NULL if not found. Requires the process table lock. */
struct x86_proc *proc_get(unsigned int pid);

/* Get the kernel process object. Does not require the process table lock. */
struct x86_proc *proc_get_kernel_proc(void);

/* Look for the first READY thread in a READY process. Requires the process table lock. */
struct x86_thread *proc_get_first_ready_thread(void);

/* Adds the given thread to the given process and sets both to READY. Requires the process table
   lock. */
void proc_add_thread(struct x86_proc *proc, struct x86_thread *thread);

/* Process list traversal. Resets the cursor to first process. Requires the process table lock. */
struct x86_proc *proc_cursor_reset(void);

/* Process list traversal. Moves cursor to next process. Requires the process table lock. */
struct x86_proc *proc_cursor_next(void);

#endif
