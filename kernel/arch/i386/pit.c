#include <kernel/cdefs.h>
#include <kernel/cpu.h>
#include <kernel/debug.h>
#include <arch/portio.h>

#define PIT_FREQ 1193180

#define PIT_CHAN(c)				(0x40 + c)
#define PIT_COMMAND				0x43
#define PIT_CHAN2_OUTPUT		0x61

#define PIT_CMD_CHAN(c)			(c << 6)
#define PIT_CMD_2BYTE			0x30
#define PIT_CMD_HW_RETR			0x02

static struct cpu_spinlock pit_spinlock;

void init_pit(void)
{
	cpu_spinlock_create(&pit_spinlock, "PIT");
}

void pit_acquire(void)
{
	cpu_spinlock_acquire(&pit_spinlock);
}

void pit_release(void)
{
	cpu_spinlock_release(&pit_spinlock);
}

void pit_prepare_counter(unsigned int microseconds)
{
	unsigned int divisor;
	uint16_t count;
	uint8_t val;

	kassert(microseconds > 0);

	divisor = 1000000 / microseconds;

	/* Avoid divisions by zero and make sure the divisor will fit within PIT's registers. */
	kassert(divisor != 0 && (PIT_FREQ / divisor) < 0xffff);

	/* Enable channel 2 of PIT. Also disables PC speaker. */
	val = pio_inb(PIT_CHAN2_OUTPUT);
	pio_outb(PIT_CHAN2_OUTPUT, (val & 0xfd) | 1);
	/* Select channel 2 of PIT in hardware retriggerable one shot mode. */
	pio_outb(PIT_COMMAND, PIT_CMD_CHAN(2) | PIT_CMD_2BYTE | PIT_CMD_HW_RETR);
	/* Program the channel with desired frequency.  */
	count = PIT_FREQ / divisor;
	pio_outb(PIT_CHAN(2), count & 0xff);
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
	while((pio_inb(PIT_CHAN2_OUTPUT) & 0x20) != 0);
}
