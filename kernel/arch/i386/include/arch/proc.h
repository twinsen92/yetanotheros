/* arch/proc.h - x86 process structures */
#ifndef ARCH_I386_PROC_H
#define ARCH_I386_PROC_H

#include <kernel/addr.h>
#include <kernel/proc.h>
#include <kernel/thread.h>

struct arch_proc
{
	/* Virtual memory. */
	struct thread_mutex pd_mutex; /* Page directory modification mutex. */
	paddr_t pd; /* Page directory used by this process. */
	vaddr_t vfrom; /* Lowest virtual memory address in use. */
	vaddr_t vto; /* Highest virtual memory address in use. */
	vaddr_t vbreak; /* Program break address. */
	vaddr_t cur_vbreak; /* Current program break address. */
};

#endif
