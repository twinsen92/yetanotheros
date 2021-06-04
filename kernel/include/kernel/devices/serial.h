/* devices/serial.h - arch independent serial interface */
#ifndef _KERNEL_DEVICES_SERIAL_H
#define _KERNEL_DEVICES_SERIAL_H

#include <kernel/exclusive_buffer.h>

struct serial;

/* Subscribe a serial input with a buffer. */
void serial_subscribe_input(struct serial *com, struct exclusive_buffer *buffer);

/* Unsubscribe a serial input with a buffer. */
void serial_unsubscribe_input(struct serial *com, struct exclusive_buffer *buffer);

/* Subscribe a serial output with a buffer. */
void serial_subscribe_output(struct serial *com, struct exclusive_buffer *buffer);

/* Unsubscribe a serial output with a buffer. */
void serial_unsubscribe_output(struct serial *com, struct exclusive_buffer *buffer);

/* Stimulate a read from a serial input. */
void serial_read(struct serial *com);

/* Stimulate a write to a serial output. */
void serial_write(struct serial *com);

#endif
