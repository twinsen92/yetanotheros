/* kernel/utils.c - some heavier kernel utils that shouldn't be inlined */

#include <kernel/cdefs.h>
#include <kernel/utils.h>

bool kstrcmp(const char *p1, const char *p2)
{
	size_t l1, l2;

	l1 = kstrlen(p1);
	l2 = kstrlen(p1);

	if (l1 != l2)
		return false;

	while (l1-- > 0)
		if (*(p1++) != *(p2++))
			return false;

	return true;
}
