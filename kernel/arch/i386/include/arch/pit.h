/* pit.h - x86 Programmable Interval Timer (Intel 8253/8254) interface */
#ifndef ARCH_I386_PIT_H
#define ARCH_I386_PIT_H

#define PIT_FREQ 1193180

#define PIT_CHAN(c)				(0x40 + c)
#define PIT_COMMAND				0x43
#define PIT_CHAN2_OUTPUT		0x61

#define PIT_CMD_CHAN(c)			(c << 6)
#define PIT_CMD_2BYTE			0x30
#define PIT_CMD_HW_RETR			0x02

void init_pit(void);

void pit_acquire(void);
void pit_release(void);

void pit_prepare_counter(unsigned int microseconds);
void pit_start_counter(void);
void pit_wait_counter(void);

#endif
