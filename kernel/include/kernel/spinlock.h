/* spinlock.h - arch-independent kernel spinlock interface */
#ifndef _KERNEL_SPINLOCK_H
#define _KERNEL_SPINLOCK_H

/* Default value for the cpu field in spinlock_t */
#define SPINLOCK_INVALID_CPU -1

/* It may happen that we don't know which CPU locked the spinlock. This can happen in early
   initialization stages, where CPUs have not yet been enumerated. */
#define SPINLOCK_UNKNOWN_CPU -2

typedef struct
{
	/* Is the lock acquired? */
	int locked;

	/* Debug: Name of the spinlock. */
	const char *name;

	/* Debug: The number of the holding CPU. */
	int cpu;
}
spinlock_t;

void spinlock_create(spinlock_t *spinlock, const char *name);
void spinlock_acquire(spinlock_t *spinlock);
void spinlock_release(spinlock_t *spinlock);
bool spinlock_held(spinlock_t *spinlock);

#endif
