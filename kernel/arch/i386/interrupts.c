/* interrupts.c - holder of interrupt handlers table */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/init.h>
#include <kernel/scheduler.h>
#include <kernel/thread.h>
#include <arch/cpu.h>
#include <arch/interrupts.h>
#include <arch/scheduler.h>

static void (*handlers[ISR_MAX])(struct isr_frame*);

/* Generic interrupt handler routine. */
void generic_interrupt_handler(struct isr_frame *frame)
{
	uint32_t cr2, cr3;
	void (*handler)(struct isr_frame*);

	asm volatile ("movl %%cr2, %0" : "=r" (cr2));
	asm volatile ("movl %%cr3, %0" : "=r" (cr3));

	kassert(frame->int_no < ISR_MAX);
	handler = handlers[frame->int_no];

	if (handler != NULL)
		handler(frame);
	else
		kpanic("unhandled interrupt");

	/* Force thread to give up CPU on clock tick. */
	if (frame->int_no == INT_IRQ_TIMER)
	{
		struct x86_thread *thread = get_current_thread();

		if(thread && thread->noarch.state == THREAD_RUNNING)
			thread_yield();
	}
}

/* Initializes the ISR registry. */
void init_isr(void)
{
	kassert(is_yaos2_initialized() == false);

	for (int i = 0; i < ISR_MAX; i++)
		handlers[i] = NULL;
}

/* Sets a handler in the ISR registry. */
void isr_set_handler(int_no_t int_no, void (*handler)(struct isr_frame*))
{
	kassert(is_yaos2_initialized() == false);
	kassert(int_no < ISR_MAX);

	handlers[int_no] = handler;
}
