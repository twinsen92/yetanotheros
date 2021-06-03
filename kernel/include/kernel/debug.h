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
#ifdef KERNEL_DEBUG
#define kassert(expr)	\
	if (!(expr))		\
		_kpanic("assertion " #expr " failed", __FILE__, __LINE__);
#else
#define kassert(expr)
#endif

#define DEBUG_MAX_CALL_DEPTH 16

struct debug_call_stack
{
	void *stack[DEBUG_MAX_CALL_DEPTH];
	int depth;
};

/* Clears and fills the call stack with current data. */
void debug_fill_call_stack(struct debug_call_stack *cs);

static inline void debug_clear_call_stack(struct debug_call_stack *cs)
{
	for (int i = 0; i < DEBUG_MAX_CALL_DEPTH; i++)
		cs->stack[i] = NULL;
	cs->depth = 0;
}

#endif
