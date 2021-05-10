/* interrupts.h - x86 interrupt structures */
#ifndef ARCH_I386_INTERRUPTS_H
#define ARCH_I386_INTERRUPTS_H

#include <kernel/addr.h>
#include <kernel/cdefs.h>

#define ISR_MAX 256

typedef uint32_t int_no_t;

typedef struct
{
	/* Pushed by intr_entry. These are the interrupted task's saved registers. */
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp_dummy;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint16_t gs, :16;
	uint16_t fs, :16;
	uint16_t es, :16;
	uint16_t ds, :16;

	/* Pushed by intrNN_stub. */
	int_no_t int_no;

	/* Sometimes pushed by the CPU, otherwise for consistency pushed as 0 by intrNN_stub.
	   The CPU puts it just under eip, but we move it here. */
	uint32_t error_code;

	/* Pushed by intrNN_stub. This frame pointer eases interpretation of backtraces.*/
	uint32_t frame_pointer;

	/* Pushed by the CPU. These are the interrupted task's saved registers. */
	uint32_t eip;
	uint16_t cs, :16;
	uint32_t eflags;
	uint32_t esp;
	uint16_t ss, :16;
} __attribute__((packed))
isr_frame_t;

/* Defined in isr_stubs.S */

extern void isr_entry(void);
extern void isr_exit(void);
extern void (*isr_stubs[ISR_MAX])(void);

/* Generic interrupt handler routine. */
void generic_interrupt_handler(isr_frame_t *frame);

/* Initializes the ISR registry. */
void init_isr(void);

/* Sets a handler in the ISR registry. */
void isr_set_handler(int_no_t int_no, void (*handler)(isr_frame_t*));

#endif