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

static inline void kmemset(void *v, char val, size_t len)
{
	char *p = v;
	while (len-- > 0)
		*(p++) = val;
}

static inline bool kstrncmp(const char *p1, const char *p2, size_t len)
{
	while (len-- > 0)
		if (*(p1++) != *(p2++))
			return false;
	return true;
}

static inline char *kstrncpy(char *p1, const char *p2, size_t len)
{
	while (len-- > 0)
		*(p1++) = *(p2++);
	return p1;
}

static inline bool kmemcmp(const void *p1, const void *p2, size_t len)
{
	return kstrncmp(p1, p2, len);
}

static inline size_t kstrlen(const char *p)
{
	size_t result = 0;
	while(*(p++) != 0)
		result++;
	return result;
}

#endif
