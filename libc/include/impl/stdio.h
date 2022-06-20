/* impl/stdio.h - libc implementation specifics for stdio.h */
#ifndef _IMPL_STDIO_H
#define _IMPL_STDIO_H 1

#include <stddef.h>

enum file_buf_mode
{
	/*
	 * Default file buffer mode, where a flush is performed when the buffer is about to be
	 * overflown.
	 */
	FBM_OVERFLOW,

	/* Flush when a newline is written. */
	FBM_NEWLINE,

	/* Always flush the buffer. This mode completely omits the buffer. */
	FBM_FLUSH,
};

typedef struct
{
	/* Constant part. */

	int fd; /* File descriptor. */
	int flags; /* Flags passed to open. */

	/* Dynamic part. Output buffer. */

	int buf_mode; /* Buffer mode. */
	char *buf; /* Buffer. */
	size_t buf_size; /* Buffer size in bytes. */
	size_t num; /* Buffer size in bytes currently in use. */
}
FILE;

#endif
