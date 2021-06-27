/* arch/proc.h - x86 process structures */
#ifndef ARCH_I386_PROC_H
#define ARCH_I386_PROC_H

#include <kernel/addr.h>
#include <kernel/proc.h>

struct arch_proc
{
	paddr_t pd; /* Page directory used by this process. */
};

#endif
