#ifndef _KERNEL_TICKS_H
#define _KERNEL_TICKS_H

#include <kernel/cdefs.h>
#include <kernel/cpu.h>
#include <kernel/debug.h>

/* TODO: Desperately need a better ticks source... */

#define TICKS_PER_SECOND		10000
#define TICKS_PER_MILLISECOND	10

extern atomic_uint_fast64_t current_ticks;

typedef uint_fast64_t ticks_t;

/* Atomically increments the current ticks count. */
#define ticks_increment() ((ticks_t)atomic_fetch_add(&current_ticks, 1))

/* Atomically fetches the current ticks count. */
#define ticks_get() ((ticks_t)atomic_load(&current_ticks))

/* Get the max number of ticks before the counter wraps. */
#define ticks_get_max() ((ticks_t)UINT_FAST64_MAX)

/* Actively wait for given milliseconds. Does not go to sleep. */
static inline void ticks_mwait(uint milliseconds)
{
	/* Check if frequency allows it. */
	kassert(TICKS_PER_SECOND > (1000 / milliseconds));

	ticks_t wanted = ticks_get() + (TICKS_PER_MILLISECOND * milliseconds);

	while (ticks_get() < wanted)
		cpu_relax();
}

#endif
