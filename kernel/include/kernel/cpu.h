/* kernel/cpu.h - arch-independent CPU interface code */
#ifndef _KERNEL_CPU_H
#define _KERNEL_CPU_H

#include <kernel/cdefs.h>
#include <kernel/debug.h>

/* Include arch-provided header. */
#include <arch/kernel/cpu.h>

/* Default value for the cpu field in struct cpu_spinlock */
#define CPU_SPINLOCK_INVALID_CPU -1

/* It may happen that we don't know which CPU locked the spinlock. This can happen in early
   initialization stages, where CPUs have not yet been enumerated. */
#define CPU_SPINLOCK_UNKNOWN_CPU -2

struct cpu_spinlock
{
	int locked; /* Is the lock acquired? */
	int num;
	int cpu; /* The number of the holding CPU. */

#ifdef KERNEL_DEBUG
	const char *name; /* Name of the CPU spinlock. */
	uint tid; /* TID of the thread the CPU was running when it was locked. */
	struct debug_call_stack lock_call_stack;
#endif
};

void cpu_spinlock_create(struct cpu_spinlock *spinlock, const char *name);
void cpu_spinlock_acquire(struct cpu_spinlock *spinlock);
void cpu_spinlock_release(struct cpu_spinlock *spinlock);
bool cpu_spinlock_held(struct cpu_spinlock *spinlock);

struct cpu_checkpoint
{
	atomic_uint num;
};

void cpu_checkpoint_create(struct cpu_checkpoint *cp);
void cpu_checkpoint_enter(struct cpu_checkpoint *cp);

#endif
