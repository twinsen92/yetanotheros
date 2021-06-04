/* pit.h - x86 Programmable Interval Timer (Intel 8253/8254) interface */
#ifndef ARCH_I386_PIT_H
#define ARCH_I386_PIT_H

void init_pit(void);

void pit_acquire(void);
void pit_release(void);

void pit_prepare_counter(unsigned int microseconds);
void pit_start_counter(void);
void pit_wait_counter(void);

#define pit_wait(microseconds)			\
	pit_prepare_counter(microseconds);	\
	pit_start_counter();				\
	pit_wait_counter()

#endif
