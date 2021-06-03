/* cpu_spinlock.h - arch-independent kernel CPU spinlock interface */
#ifndef _KERNEL_CPU_SPINLOCK_H
#define _KERNEL_CPU_SPINLOCK_H

#include <kernel/debug.h>

/* Default value for the cpu field in struct cpu_spinlock */
#define CPU_SPINLOCK_INVALID_CPU -1

/* It may happen that we don't know which CPU locked the spinlock. This can happen in early
   initialization stages, where CPUs have not yet been enumerated. */
#define CPU_SPINLOCK_UNKNOWN_CPU -2

struct cpu_spinlock
{
	/* Is the lock acquired? */
	int locked;

	int cpu; /* The number of the holding CPU. */

#ifdef KERNEL_DEBUG
	const char *name; /* Name of the CPU spinlock. */

	struct debug_call_stack lock_call_stack;
#endif
};

void cpu_spinlock_create(struct cpu_spinlock *spinlock, const char *name);
void cpu_spinlock_acquire(struct cpu_spinlock *spinlock);
void cpu_spinlock_release(struct cpu_spinlock *spinlock);
bool cpu_spinlock_held(struct cpu_spinlock *spinlock);

#endif
