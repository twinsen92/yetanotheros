
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/exclusive_buffer.h>
#include <kernel/heap.h>
#include <kernel/thread.h>
#include <kernel/utils.h>

static inline void unsafe_assert_valid(struct exclusive_buffer *buffer)
{
	kassert(buffer->head > buffer->buffer);
	kassert(buffer->head <= buffer->tail);
	kassert(buffer->tail <= buffer->buffer + buffer->buffer_size);
}

static inline size_t unsafe_get_contents_length(struct exclusive_buffer *buffer)
{
	return (size_t)(buffer->tail - buffer->head);
}

static void unsafe_move_head(struct exclusive_buffer *buffer)
{
	size_t contents;

	/* Ignore unneccessary calls. */
	if (buffer->head == buffer->buffer)
		return;

	unsafe_assert_valid(buffer);

	contents = unsafe_get_contents_length(buffer);

	for (size_t i = 0; i < contents; i++)
		buffer->buffer[i] = buffer->head[i];

	buffer->head = buffer->buffer;
	buffer->tail = buffer->buffer + contents;
}

/* Creates a buffer object. Allocates the underlying array. */
void eb_create(struct exclusive_buffer *buffer, size_t size, uintptr_t alignment, int flags)
{
	buffer->flags = flags | EBF_EOF;
	buffer->alignment = kmax(alignment, 1);
	buffer->buffer_size = size;
	buffer->buffer = kalloc(HEAP_NORMAL, buffer->alignment, size);
	buffer->head = buffer->buffer;
	buffer->tail = buffer->buffer;

	thread_mutex_create(&(buffer->mutex));
	thread_cond_create(&(buffer->changed));
}

/* Locks the buffer. Frees underlying array and invalidates pointers. */
void eb_free(struct exclusive_buffer *buffer)
{
	/* Acquire the mutex to make sure no one is holding it. We will not release it which will lock
	   threads still trying to use the buffer. */
	thread_mutex_acquire(&(buffer->mutex));

	kfree(buffer->buffer);

	/* Set the pointers to NULL, just to be extra sure... */
	buffer->buffer = NULL;
	buffer->head = NULL;
	buffer->tail = NULL;
}

/* Reads a requested number of bytes. Waits until all of the bytes have been read into dest. */
void eb_read(struct exclusive_buffer *buffer, uint8_t *dest, size_t size)
{
	size_t num_read = 0, batch;

	thread_mutex_acquire(&(buffer->mutex));

	while (num_read < size)
	{
		/* Make sure data is available. Wait for data otherwise. */
		while (buffer->flags & EBF_EOF)
			thread_cond_wait(&(buffer->changed), &(buffer->mutex));

		/* Calculate how much we're gonna read, read the bytes and modify the buffer. */
		batch = kmin(unsafe_get_contents_length(buffer), size - num_read);
		kmemmove(dest + num_read, buffer->head, batch);
		buffer->head += batch;
		num_read += batch;
	}

	thread_mutex_release(&(buffer->mutex));
}

/* Tries to read at most size bytes. Returns the acutal number of read bytes. */
size_t eb_try_read(struct exclusive_buffer *buffer, uint8_t *dest, size_t size)
{
	size_t num_read = 0;

	thread_mutex_acquire(&(buffer->mutex));

	if (buffer->flags & EBF_EOF)
		return 0;

	/* Calculate how much we're gonna read, read the bytes and modify the buffer. */
	num_read = kmin(unsafe_get_contents_length(buffer), size);
	kmemmove(dest, buffer->head, num_read);
	buffer->head += num_read;

	thread_mutex_release(&(buffer->mutex));

	return num_read;
}

static void unsafe_growing_write(struct exclusive_buffer *buffer, const uint8_t *src, size_t size)
{
	size_t new_size;

	unsafe_move_head(buffer);

	new_size = unsafe_get_contents_length(buffer) + size;
	buffer->buffer = krealloc(buffer->buffer, HEAP_NORMAL, buffer->alignment, new_size);
	kmemcpy(buffer->tail, src, size);
	buffer->tail += size;
}

/* Writes size bytes to the buffer. Waits until all data has been written to the buffer. */
void eb_write(struct exclusive_buffer *buffer, const uint8_t *src, size_t size)
{
	size_t num_written = 0, batch;

	thread_mutex_acquire(&(buffer->mutex));

	if (buffer->flags & EBF_GROWING)
	{
		unsafe_growing_write(buffer, src, size);
		goto _eb_write_done;
	}

	while (num_written < size)
	{
		unsafe_move_head(buffer);

		/* Make sure some space is available. Wait for changes otherwise. */
		while (buffer->tail == buffer->buffer + buffer->buffer_size)
			thread_cond_wait(&(buffer->changed), &(buffer->mutex));

		unsafe_move_head(buffer);

		/* Calculate how much we're gonna write, write the bytes and modify the buffer. */
		batch = kmin(buffer->buffer_size - unsafe_get_contents_length(buffer), size - num_written);
		kmemcpy(buffer->tail, src + num_written, batch);
		buffer->tail += batch;
		num_written += batch;
	}

_eb_write_done:
	thread_mutex_release(&(buffer->mutex));
}

/* Tries to write at most size bytes. Returns the actual number of bytes written. */
size_t eb_try_write(struct exclusive_buffer *buffer, const uint8_t *src, size_t size)
{
	size_t num_written = 0;

	thread_mutex_acquire(&(buffer->mutex));

	if (buffer->flags & EBF_GROWING)
	{
		unsafe_growing_write(buffer, src, size);
		num_written = size; /* TODO: This is probably not safe to assume... */
		goto _eb_try_write_done;
	}

	unsafe_move_head(buffer);

	/* Calculate how much we're gonna write, write the bytes and modify the buffer. */
	num_written = kmin(buffer->buffer_size - unsafe_get_contents_length(buffer), size);
	kmemcpy(buffer->tail, src, num_written);
	buffer->tail += num_written;

_eb_try_write_done:
	thread_mutex_release(&(buffer->mutex));

	return num_written;
}

/* Waits until EBF_EOF has been set. */
void eb_flush(struct exclusive_buffer *buffer)
{
	thread_mutex_acquire(&(buffer->mutex));

	while ((buffer->flags & EBF_EOF) == 0)
		thread_cond_wait(&(buffer->changed), &(buffer->mutex));

	thread_mutex_release(&(buffer->mutex));
}
