/*
	debug.h - debug functions

	These are guarenteed be usable in any state of the kernel, except:
		1. early boot stage, where all of kernel's executable might not be mapped yet,
		2. before init_debug() is called.
*/
#ifndef _KERNEL_DEBUG_H
#define _KERNEL_DEBUG_H

#include <kernel/cdefs.h>

/* Initializes the debug subsystem in the kernel. Call this early in initialization. */
void init_debug(void);

/* Debug formatted printer. */
int kdprintf(const char* __restrict, ...);

noreturn _kpanic(const char *reason, const char *file, unsigned int line);

/* Kernel panic, with a reason. */
#define kpanic(reason) _kpanic(reason, __FILE__, __LINE__);

/* Kernel abort, with unknown reason. */
#define kabort() _kpanic("aborted", __FILE__, __LINE__);

/* Kernel assertion that the expression is true. If it is not, the kernel panics. */
#define kassert(expr)	\
	if (!(expr))		\
		_kpanic("assertion " #expr " failed", __FILE__, __LINE__);

#endif
