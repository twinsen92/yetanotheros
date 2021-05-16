/* proc.h - x86 process structures */
#ifndef ARCH_I386_PROC_H
#define ARCH_I386_PROC_H

#include <kernel/addr.h>
#include <kernel/queue.h>
#include <kernel/proc.h>
#include <kernel/thread.h>
#include <arch/thread.h>

struct x86_proc
{
	paddr_t pd; /* Page directory used by this process. */

	struct x86_thread_list threads; /* Thread list. */
	struct proc noarch; /* Arch-independent structure. */

	LIST_ENTRY(x86_proc) pointers;
};

LIST_HEAD(x86_proc_list, x86_proc);

#endif
