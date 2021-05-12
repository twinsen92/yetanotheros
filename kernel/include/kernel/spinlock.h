/* spinlock.h - arch-independent kernel spinlock interface */
#ifndef _KERNEL_SPINLOCK_H
#define _KERNEL_SPINLOCK_H

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
