
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <yaos2/kernel/defs.h>
#include <yaos2/kernel/errno.h>

#include "nprintf.h"

/* TODO: Implement and use ferror where appropriate. */

#define min(x, y) ((x) > (y) ? (y) : (x))

#define DEFAULT_BUF_MODE FBM_NEWLINE
#define DEFAULT_BUF_SIZE BUFSIZ

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

FILE *fopen(const char *path, const char *mode)
{
	int fd;
	FILE *f;

	/* TODO: Use the file mode. */

	fd = open(path, 0);

	if (fd < 0)
		return NULL;

	f = malloc(sizeof(FILE));
	f->fd = fd;
	f->flags = 0;
	f->buf_mode = DEFAULT_BUF_MODE;
	f->buf_foreign = 0;
	f->buf = malloc(DEFAULT_BUF_SIZE);
	f->buf_size = DEFAULT_BUF_SIZE;
	f->num = 0;

	return f;
}

int fclose(FILE *stream)
{
	int fd, ret;

	fd = stream->fd;

	if (!(stream->buf_foreign))
		free(stream->buf);
	free(stream);

	ret = close(fd);

	if (ret < 0)
	{
		errno = -ret;
		return EOF;
	}

	return 0;
}

void setbuf(FILE *stream, char *buf)
{
	if (!(stream->buf_foreign))
		free(stream->buf);

	stream->buf = buf;
	stream->buf_foreign = 1;
}

int fseek(FILE *stream, long int offset, int origin)
{
	long int ret;

	ret = lseek(stream->fd, offset, origin);

	if (ret < 0)
	{
		errno = -ret;
		return -1;
	}

	return 0;
}

long int ftell(FILE *stream)
{
	long int ret;

	ret = lseek(stream->fd, 0, SEEK_CUR);

	if (ret < 0)
	{
		errno = -ret;
		return -1;
	}

	return ret;
}

int fputc(int character, FILE *stream)
{
	char c;
	int ret;

	c = (char)character;
	ret = fwrite(&c, 1, 1, stream);

	/* On error just return EOF, fwrite will have set the errno. */
	if (ret < 0)
		return EOF;

	return character;
}

int fputs(const char *str, FILE *stream)
{
	int ret;

	ret = fwrite(str, 1, strlen(str), stream);

	/* On error just return EOF, fwrite will have set the errno. */
	if (ret < 0)
		return EOF;

	return ret;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	ssize_t ret;

	if (size == 0 || nmemb == 0)
		return 0;

	ret = read(stream->fd, ptr, size * nmemb);

	if (ret < 0)
	{
		errno = -ret;
		return 0;
	}

	return size * nmemb;
}

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
		return 0;

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

static int stream_put_many(struct generic_printer *printer, const char *str, size_t len)
{
	FILE *stream = (FILE*)printer->opaque;
	return fwrite(str, 1, len, stream);
}

static int stream_put_one(struct generic_printer *printer, char c)
{
	return stream_put_many(printer, &c, 1);
}

static inline void build_generic_printer(struct generic_printer *printer, FILE *stream)
{
	printer->opaque = stream;
	printer->put_one = stream_put_one;
	printer->put_many = stream_put_many;
}

int fprintf(FILE *stream, const char *format, ...)
{
	int ret;
	va_list parameters;
	struct generic_printer printer;

	build_generic_printer(&printer, stream);
	va_start(parameters, format);
	ret = generic_nprintf(&printer, NULL, -1, format, parameters);
	va_end(parameters);

	return ret;
}

int vfprintf(FILE *stream, const char *format, va_list arg)
{
	int ret;
	struct generic_printer printer;

	build_generic_printer(&printer, stream);
	ret = generic_nprintf(&printer, NULL, -1, format, arg);

	return ret;
}
