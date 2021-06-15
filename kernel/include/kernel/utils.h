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

static inline void kmemmove(void *v1, void *v2, size_t len)
{
	char *p1 = v1;
	char *p2 = v2;
	while (len-- > 0)
	{
		*p1 = *p2;
		*p2 = 0;
		p1++;
		p2++;
	}
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

static inline void *kmemcpy(void *v1, const void *v2, size_t len)
{
	return (void*)kstrncpy(v1, v2, len);
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

static inline char *kstrcpy(char *to, const char *from)
{
	kstrncpy(to, from, kstrlen(from));
	return to;
}

static inline const char *kstrchr(const char *str, int chr)
{
	while(*str != '\0')
	{
		if (*str == chr)
			return str;
		str++;
	}

	return NULL;
}

static inline char *kstrcat(char *to, const char *from)
{
	kstrcpy(to + kstrlen(to), from);
	return to;
}

static inline const void *kmemchr(const void *v, int val, size_t num)
{
	const byte *p = v;

	while(num-- > 0)
	{
		if (*p == val)
			return p;
		p++;
	}

	return NULL;
}

#define kmin(x, y) ((x) > (y) ? (y) : (x))
#define kmax(x, y) ((x) > (y) ? (x) : (y))

#endif
