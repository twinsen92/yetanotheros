#include <stdio.h>
#include <stdlib.h>

void _assert_failed(const char *expr, const char *file, int line)
{
	/* TODO: Maybe print this to stderr? */
	printf("assertion %s failed at %s:%d", expr, file, line);
	abort();
}
