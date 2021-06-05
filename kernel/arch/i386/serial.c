
#include <kernel/cdefs.h>
#include <kernel/cpu.h>
#include <kernel/debug.h>
#include <kernel/exclusive_buffer.h>
#include <kernel/proc.h>
#include <kernel/scheduler.h>
#include <kernel/thread.h>
#include <kernel/utils.h>
#include <arch/cpu.h>
#include <arch/interrupts.h>
#include <arch/serial.h>
#include <arch/cpu/apic.h>
#include <arch/kernel/portio.h>

/* TODO: Use linked list. */
#define MAX_SUBSCRIBERS 8

struct serial
{
	uint16_t port;


	tid_t handler;
	struct thread_mutex mutex;
	struct thread_cond data_available;

	struct exclusive_buffer *input_subscribers[MAX_SUBSCRIBERS];
	struct exclusive_buffer *output_subscribers[MAX_SUBSCRIBERS];
};

static struct serial com1;
static struct serial com2;

/* Serial ports I/O */

static inline bool serial_break(uint16_t port)
{
	return (pio_inb(COM_LINE_STS_REG(port)) & 0x10) != 0;
}

static inline bool serial_received(uint16_t port)
{
	return !serial_break(port) && (pio_inb(COM_LINE_STS_REG(port)) & 1) != 0;
}

static inline uint8_t read_serial(uint16_t port)
{
	return pio_inb(COM_DATA_REGISTER(port));
}

static inline bool is_transmit_empty(uint16_t port)
{
	return (pio_inb(COM_LINE_STS_REG(port)) & 0x20) != 0;
}

static inline void write_serial(uint16_t port, uint8_t byte)
{
	pio_outb(COM_DATA_REGISTER(port), byte);
}

static void try_serial_write(struct serial *s)
{
	int i, p;
	bool read;
	uint8_t data[COM_FIFO_SIZE];
	struct exclusive_buffer *eb;

	/* Check for break interrupt bit. If set, it means there's likely nothing on the other side. */
	if (serial_break(s->port))
		return;

	if (is_transmit_empty(s->port))
	{
		/* Fill the buffer from subscriber buffers. Try to get a byte from each subscriber. */
		for (p = 0; p < COM_FIFO_SIZE; p++)
		{
			read = false;

			for (i = 0; i < MAX_SUBSCRIBERS; i++)
			{
				eb = s->output_subscribers[i];

				if (eb == NULL)
					continue;

				if (eb_try_read(eb, &(data[p]), 1))
				{
					read = true;
					break;
				}
			}

			if (!read)
				break;
		}

		/* Actually transmit from the buffer byte by byte */
		for (i = 0; i < p; i++)
			write_serial(s->port, data[i]);
	}
}

static void try_serial_read(struct serial *s)
{
	uint8_t data;
	struct exclusive_buffer *eb;

	/* Check for break interrupt bit. If set, it means there's likely nothing on the other side. */
	if (serial_break(s->port))
		return;

	while (serial_received(s->port))
	{
		data = read_serial(s->port);

		for (int i = 0; i < MAX_SUBSCRIBERS; i++)
		{
			eb = s->output_subscribers[i];

			if (eb == NULL)
				continue;

			eb_try_write(eb, &data, 1);
		}
	}
}

/* Interrupt handling */

static void com_irq_handler(struct serial *s, __unused struct isr_frame *frame)
{
	thread_cond_notify(&(s->data_available));
	lapic_eoi();
}

static void com1_irq_handler(struct isr_frame *frame)
{
	com_irq_handler(&com1, frame);
}

static void com2_irq_handler(struct isr_frame *frame)
{
	com_irq_handler(&com2, frame);
}

/* Subscription handling */

static void subscribe(struct exclusive_buffer **subs, struct exclusive_buffer *eb)
{
	int i = 0;

	for (i = 0; i < MAX_SUBSCRIBERS; i++)
	{
		if (subs[i] == NULL)
		{
			subs[i] = eb;
			break;
		}
	}

	/* Check if we have registered. */
	if (i == MAX_SUBSCRIBERS)
		kpanic("subscribe(): exceeded max subscribers");
}

static void unsubscribe(struct exclusive_buffer **subs, struct exclusive_buffer *eb)
{
	for (int i = 0; i < MAX_SUBSCRIBERS; i++)
		if (subs[i] == eb)
			subs[i] = NULL;
}

static void serial_handler_main(void *cookie)
{
	struct serial *s = (struct serial *)cookie;

	while (1)
	{
		thread_mutex_acquire(&(s->mutex));
		/* TODO: Is disabling interrupts here really necessary? */
		push_no_interrupts();

		/* Wait until we have data available. */
		while (!serial_received(s->port) && !is_transmit_empty(s->port))
			thread_cond_wait(&(s->data_available), &(s->mutex));

		if (serial_received(s->port))
			try_serial_read(s);

		if (is_transmit_empty(s->port))
			try_serial_write(s);

		pop_no_interrupts();
		thread_mutex_release(&(s->mutex));
	}
}

/* Initialization */

static void init_port(struct serial *s, uint16_t port, uint16_t divisor)
{
	s->port = port;
	thread_cond_create(&(s->data_available));
	thread_mutex_create(&(s->mutex));

	/* Initialize the controller. */
	/* TODO: Magic numbers... */
	pio_outb(COM_INT_ENABL_REG(port), 0); /* Disable interrupts. */
	pio_outb(COM_LINE_CTL_REG(port), COM_DLAB_BIT); /* Enable DLAB */
	pio_outb(COM_DIVISOR_LOW(port), divisor); /* Divisor lower 8 bits */
	pio_outb(COM_DIVISOR_HIGH(port), divisor >> 8); /* Divisor higher 8 bits */
	pio_outb(COM_LINE_CTL_REG(port), COM_8_BITS | COM_NO_PARITY_BITS | COM_1_STOP_BIT); /* 8/N/1 */
	pio_outb(COM_INT_FIFO_REG(port), 0xc7); /* Enable FIFO, 14 byte treshold (?). */
	pio_outb(COM_MODM_CTL_REG(port), 0x0b); /* IRQs enabled, RTS/DSR set (?). */
	pio_outb(COM_INT_ENABL_REG(port), COM_IER_INPUT_BIT | COM_IER_NO_OUTPUT_BIT); /* Enable some interrupts. */

	/* Create handler thread. */
	s->handler = thread_create(PID_KERNEL, serial_handler_main, s, "serial port handler");
}

/* arch/serial.h interface */

void init_serial(void)
{
	init_port(&com1, COM1, COM_DEFAULT_DIVISOR);
	init_port(&com2, COM2, COM_DEFAULT_DIVISOR);

	isr_set_handler(INT_IRQ_COM1, com1_irq_handler);
	isr_set_handler(INT_IRQ_COM2, com2_irq_handler);

	ioapic_clear_mask(INT_IRQ_COM1);
	ioapic_clear_mask(INT_IRQ_COM2);
}

struct serial *serial_get_com1(void)
{
	return &com1;
}

struct serial *serial_get_com2(void)
{
	return &com2;
}

/* kernel/devices/serial.h interface */

void serial_subscribe_input(struct serial *com, struct exclusive_buffer *buffer)
{
	thread_mutex_acquire(&(com->mutex));
	subscribe(com->input_subscribers, buffer);
	thread_mutex_release(&(com->mutex));
}

void serial_unsubscribe_input(struct serial *com, struct exclusive_buffer *buffer)
{
	thread_mutex_acquire(&(com->mutex));
	unsubscribe(com->input_subscribers, buffer);
	thread_mutex_release(&(com->mutex));
}

void serial_subscribe_output(struct serial *com, struct exclusive_buffer *buffer)
{
	thread_mutex_acquire(&(com->mutex));
	subscribe(com->output_subscribers, buffer);
	thread_mutex_release(&(com->mutex));
}

void serial_unsubscribe_output(struct serial *com, struct exclusive_buffer *buffer)
{
	thread_mutex_acquire(&(com->mutex));
	unsubscribe(com->output_subscribers, buffer);
	thread_mutex_release(&(com->mutex));
}

void serial_read(struct serial *com)
{
	thread_cond_notify(&(com->data_available));
}

void serial_write(struct serial *com)
{
	thread_cond_notify(&(com->data_available));
}
