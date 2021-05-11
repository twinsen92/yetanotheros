/* utils.h - utility functions */
#ifndef _KERNEL_UTILS_H
#define _KERNEL_UTILS_H

#include <kernel/cdefs.h>

/* Memset for a volatile region in memory. */
static inline void memset_volatile(volatile void *v, char val, size_t len)
{
	volatile char *p = v;
	while (len-- > 0)
		*(p++) = val;
}

#endif
