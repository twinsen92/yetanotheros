/* interrupts.h - arch-independent interrupt interface */
#ifndef _KERNEL_INTERRUPTS_H
#define _KERNEL_INTERRUPTS_H

#include <kernel/cdefs.h>

void push_no_interrupts(void);
void pop_no_interrupts(void);

#endif
