/* spinlock.h - arch-independent kernel spinlock interface */
#ifndef _KERNEL_SPINLOCK_H
#define _KERNEL_SPINLOCK_H

/* Default value for the cpu field in struct spinlock */
#define SPINLOCK_INVALID_CPU -1

/* It may happen that we don't know which CPU locked the spinlock. This can happen in early
   initialization stages, where CPUs have not yet been enumerated. */
#define SPINLOCK_UNKNOWN_CPU -2

struct spinlock
{
	/* Is the lock acquired? */
	int locked;

	/* Debug: Name of the spinlock. */
	const char *name;

	/* Debug: The number of the holding CPU. */
	int cpu;
};

void spinlock_create(struct spinlock *spinlock, const char *name);
void spinlock_acquire(struct spinlock *spinlock);
void spinlock_release(struct spinlock *spinlock);
bool spinlock_held(struct spinlock *spinlock);

#endif
