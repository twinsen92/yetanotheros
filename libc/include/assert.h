#ifndef _ASSERT_H
#define _ASSERT_H 1

void _assert_failed(const char *expr, const char *file, int line);

#define assert(expr)	\
	if (!(expr))		\
		_assert_failed(#expr, __FILE__, __LINE__)

#endif
