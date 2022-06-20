
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <yaos2/kernel/errno.h>

#define min(x, y) ((x) > (y) ? (y) : (x))

#define DEFAULT_BUF_MODE FBM_NEWLINE
#define DEFAULT_BUF_SIZE 128

static char static_stdin_buf[DEFAULT_BUF_SIZE];
static FILE static_stdin = {
		.fd = STDIN_FILENO,
		.flags = 0,

		.buf_mode = DEFAULT_BUF_MODE,
		.buf = static_stdin_buf,
		.buf_size = DEFAULT_BUF_SIZE,
		.num = 0,
};

static char static_stdout_buf[DEFAULT_BUF_SIZE];
static FILE static_stdout = {
		.fd = STDOUT_FILENO,
		.flags = 0,

		.buf_mode = DEFAULT_BUF_MODE,
		.buf = static_stdout_buf,
		.buf_size = DEFAULT_BUF_SIZE,
		.num = 0,
};

static char static_stderr_buf[DEFAULT_BUF_SIZE];
static FILE static_stderr = {
		.fd = STDERR_FILENO,
		.flags = 0,

		.buf_mode = DEFAULT_BUF_MODE,
		.buf = static_stderr_buf,
		.buf_size = DEFAULT_BUF_SIZE,
		.num = 0,
};

FILE *stdin = &static_stdin;
FILE *stdout = &static_stdout;
FILE *stderr = &static_stderr;

static size_t fwrite_flush(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	ssize_t ret = write(stream->fd, ptr, size * nmemb);

	if (ret < 0)
	{
		errno = -ret;
		return 0;
	}

	return ret / size;
}

static size_t fwrite_newline(const char *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t total_size = size * nmemb;
	ssize_t ret;
	size_t batch;
	const char *loc = NULL;
	size_t written_size = 0;

	while (1)
	{
		/* Flush if the stream would overflow, or if we have found a newline. */
		if (loc || stream->num == stream->buf_size)
		{
			ret = write(stream->fd, stream->buf, stream->num);

			if (ret < 0)
			{
				errno = -ret;
				return 0;
			}

			/* TODO: What do we do if we didn't write all of the data? */
			stream->num = 0;
		}

		if (total_size == 0)
			break;

		/* Calculate how much we can fit into the buffer. */
		batch = min(total_size, stream->buf_size - stream->num);

		/* Look for a newline character in the data we would write. */
		loc = memchr(ptr + written_size, '\n', batch);

		/* If we found it, calculate the minimum batch size needed to put the newline in the buffer. */
		if (loc)
			batch = loc - (ptr + written_size) + 1;

		memcpy(stream->buf + stream->num, ptr + written_size, batch);

		total_size -= batch;
		written_size += batch;
		stream->num += batch;
	}

	return written_size / nmemb;
}

static size_t fwrite_overflow(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t total_size = size * nmemb;
	ssize_t ret;
	size_t batch;
	size_t written_size = 0;

	while (1)
	{
		/* Flush if the stream would overflow, or if we have found a newline. */
		if (stream->num == stream->buf_size)
		{
			ret = write(stream->fd, stream->buf, stream->num);

			if (ret < 0)
			{
				errno = -ret;
				return 0;
			}

			/* TODO: What do we do if we didn't write all of the data? */
			stream->num = 0;
		}

		if (total_size == 0)
			break;

		/* Calculate how much we can fit into the buffer. */
		batch = min(total_size, stream->buf_size - stream->num);

		memcpy(stream->buf + stream->num, ptr + written_size, batch);

		total_size -= batch;
		written_size += batch;
		stream->num += batch;
	}

	return written_size / nmemb;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	if (size == 0 || nmemb == 0)
	{
		errno = 0;
		return 0;
	}

	if (stream->buf_mode == FBM_FLUSH)
		return fwrite_flush(ptr, size, nmemb, stream);
	else if (stream->buf_mode == FBM_NEWLINE)
		return fwrite_newline(ptr, size, nmemb, stream);
	else if (stream->buf_mode == FBM_OVERFLOW)
		return fwrite_overflow(ptr, size, nmemb, stream);

	errno = EUNSPEC;
	return 0;
}

int fflush(FILE *stream)
{
	ssize_t ret;

	ret = write(stream->fd, stream->buf, stream->num);

	if (ret < 0)
	{
		errno = -ret;
		return EOF;
	}

	stream->num = 0;

	return 0;
}
