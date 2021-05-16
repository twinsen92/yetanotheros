#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/spinlock.h>
#include <arch/pit.h>
#include <arch/portio.h>

static struct spinlock pit_spinlock;

void init_pit(void)
{
	spinlock_create(&pit_spinlock, "PIT");
}

void pit_acquire(void)
{
	spinlock_acquire(&pit_spinlock);
}

void pit_release(void)
{
	spinlock_release(&pit_spinlock);
}

void pit_prepare_counter(unsigned int microseconds)
{
	unsigned int divisor;
	uint32_t count;
	uint8_t val;

	kassert(microseconds > 0);

	divisor = 1000000 / microseconds;

	/* Avoid divisions by zero and make sure the divisor will fit within PIT's registers. */
	kassert(divisor != 0 && (PIT_FREQ / divisor) < 0xffff);

	/* Enable channel 2 of PIT. */
	val = pio_inb(PIT_CHAN2_OUTPUT);
	pio_outb(PIT_CHAN2_OUTPUT, (val & 0xfd) | 1);
	/* Select channel 2 of PIT in hardware retriggerable one shot mode. */
	pio_outb(PIT_COMMAND, PIT_CMD_CHAN(2) | PIT_CMD_2BYTE | PIT_CMD_HW_RETR);
	/* Program the channel with desired frequency.  */
	count = PIT_FREQ / divisor;
	pio_outb(PIT_CHAN(2), count);
	pio_outb(PIT_CHAN(2), count >> 8);
}

void pit_start_counter(void)
{
	uint8_t val;
	/* Reset the channel 2 counter and start counting. */
	val = pio_inb(PIT_CHAN2_OUTPUT);
	pio_outb(PIT_CHAN2_OUTPUT, val & 0xfe);
	pio_outb(PIT_CHAN2_OUTPUT, val | 1);
}

void pit_wait_counter(void)
{
	/* Wait for PIT to stop counting. */
	while((pio_inb(PIT_CHAN2_OUTPUT) & 0x20) == 0);
}