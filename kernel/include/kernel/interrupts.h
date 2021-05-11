/* interrupts.h - arch-independent interrupt interface */
#ifndef _KERNEL_INTERRUPTS_H
#define _KERNEL_INTERRUPTS_H

#include <kernel/cdefs.h>

/* Pushes a "cli" onto the CPU's interrupt flag stack. */
void push_no_interrupts(void);

/* Pops a "cli" from the CPU's interrupt flag stack. */
void pop_no_interrupts(void);

#endif
