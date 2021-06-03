#ifndef ARCH_I386_SERIAL_H
#define ARCH_I386_SERIAL_H

#include <kernel/cpu_spinlock.h>
#include <kernel/exclusive_buffer.h>

#define COM1 0x3f8
#define COM2 0x2f8

#define COM_DEFAULT_DIVISOR 3

/* Macros */
#define COM_DATA_REGISTER(com) (com)
#define COM_INT_ENABL_REG(com) (com + 1)

/* With DLAB set to 1. */
#define COM_DIVISOR_LOW(com) (com)
#define COM_DIVISOR_HIGH(com) (com + 1)

#define COM_INT_FIFO_REG(com) (com + 2)
#define COM_LINE_CTL_REG(com) (com + 3)
#define COM_MODM_CTL_REG(com) (com + 4)
#define COM_LINE_STS_REG(com) (com + 5)
#define COM_MODM_STS_REG(com) (com + 6)
#define COM_SCRATCH_REG(com) (com + 7)

/* Line control register bits */
#define COM_DLAB_BIT 0x80

#define COM_5_BITS 0x00
#define COM_6_BITS 0x01
#define COM_7_BITS 0x02
#define COM_8_BITS 0x03

#define COM_1_STOP_BIT 0x00
#define COM_2_STOP_BIT 0x04

#define COM_NO_PARITY_BITS 0x00
#define COM_ODD_PARITY_BITS 0x08
#define COM_EVEN_PARITY_BITS 0x18
#define COM_MARK_PARITY_BITS 0x28
#define COM_SPACE_PARITY_BITS 0x38

/* Interrupt enable register bits */
#define COM_IER_INPUT_BIT 0x01
#define COM_IER_NO_OUTPUT_BIT 0x02
#define COM_IER_BREAK_BIT 0x04
#define COM_IER_STATUS_BIT 0x08

#define COM_FIFO_SIZE 14

struct serial;

void init_serial(void);

struct serial *serial_get_com1(void);
struct serial *serial_get_com2(void);

void serial_subscribe_input(struct serial *com, struct exclusive_buffer *buffer);
void serial_unsubscribe_input(struct serial *com, struct exclusive_buffer *buffer);
void serial_subscribe_output(struct serial *com, struct exclusive_buffer *buffer);
void serial_unsubscribe_output(struct serial *com, struct exclusive_buffer *buffer);

void serial_read(struct serial *com);
void serial_write(struct serial *com);

#endif
