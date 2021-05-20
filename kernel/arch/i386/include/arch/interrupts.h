/* interrupts.h - x86 interrupt structures */
#ifndef ARCH_I386_INTERRUPTS_H
#define ARCH_I386_INTERRUPTS_H

#include <kernel/addr.h>
#include <kernel/cdefs.h>

#define INT_PAGEFAULT		0x0E

#define INT_IRQ0			0x20
#define INT_IRQ_TIMER		INT_IRQ0
#define INT_IRQ_ERROR		(INT_IRQ0 + 19)
#define INT_IRQ_SPURIOUS	(INT_IRQ0 + 31)

/* Interrupt vector for a system call from user code. */
#define INT_SYSCALL			0x80

/* Interrupt vector for an IPI that commands a TLB flush due to page tables update. */
#define INT_FLUSH_TLB_IPI	0x81

#define ISR_MAX 256

typedef uint32_t int_no_t;

packed_struct isr_frame
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
};

/* Defined in isr_stubs.S */

extern void isr_entry(void);
extern void isr_exit(void);
extern void (*isr_stubs[ISR_MAX])(void);

/* Generic interrupt handler routine. */
void generic_interrupt_handler(struct isr_frame *frame);

/* Initializes the ISR registry. */
void init_isr(void);

/* Sets a handler in the ISR registry. */
void isr_set_handler(int_no_t int_no, void (*handler)(struct isr_frame*));

#endif
