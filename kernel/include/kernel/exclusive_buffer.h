/*
	exclusive_buffer.h - generic buffer with exclusive, but preemptible write and read

		A generic buffer object. Used in a subscriber/subscribee model. An example of usage is
	communication using the serial port. A subscriber can read or write using a buffer object and
	the serial port driver (the subscribee) can asynchronously, using deferrable tasks, read
	or write to it.

	 NOTE: This object can only be used from within a thread.
*/
#ifndef _KERNEL_EXCLUSIVE_BUFFER_H
#define _KERNEL_EXCLUSIVE_BUFFER_H

#include <kernel/cdefs.h>
#include <kernel/heap.h>
#include <kernel/thread.h>

/* Exclusive buffer flags */

#define EBF_GROWING		0x02 /* Buffer can be reallocated to store more data in it. */
#define EBF_EOF			0x04 /* No data is remaining in the read buffer. */
#define EBF_OVERFLOW	0x08 /* Read buffer has overflowed. */

struct exclusive_buffer
{
	int flags; /* Buffer flags. */
	uintptr_t alignment; /* Buffer alignment. */

	/* TODO: Add page directory? */

	size_t buffer_size; /* Maximum buffer size. */
	uint8_t *buffer; /* Pointer to the buffer base. */

	uint8_t *head; /* Current buffer head. */
	uint8_t *tail; /* Current buffer tail. */

	struct thread_mutex mutex; /* Buffer exclusive lock. */
	struct thread_cond changed; /* Buffer changed wait condition. */
};

/* Creates a buffer object. Allocates the underlying array. */
void eb_create(struct exclusive_buffer *buffer, size_t size, uintptr_t alignment, int flags);

/* Locks the buffer. Frees underlying array and invalidates pointers. */
void eb_free(struct exclusive_buffer *buffer);

/* Reads a requested number of bytes. Waits until all of the bytes have been read into dest. */
void eb_read(struct exclusive_buffer *buffer, uint8_t *dest, size_t size);

/* Tries to read at most size bytes. Returns the acutal number of read bytes. */
size_t eb_try_read(struct exclusive_buffer *buffer, uint8_t *dest, size_t size);

/* Writes size bytes to the buffer. Waits until all data has been written to the buffer. */
void eb_write(struct exclusive_buffer *buffer, const uint8_t *src, size_t size);

/* Tries to write at most size bytes. Returns the actual number of bytes written. */
size_t eb_try_write(struct exclusive_buffer *buffer, const uint8_t *src, size_t size);

/* Waits until EBF_EOF has been set. */
void eb_flush(struct exclusive_buffer *buffer);

#endif
