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
	struct x86_cpu *cpu;
	void (*handler)(struct isr_frame*);

	kassert(frame->int_no < ISR_MAX);
	handler = handlers[frame->int_no];

	if (handler != NULL)
		handler(frame);
	else
		kpanic("unhandled interrupt");

	/* Force thread to give up CPU on clock tick. */
	if (frame->int_no == INT_IRQ_TIMER)
	{
		cpu = cpu_current();

		/* Do not give up CPU if we have disabled preemption. */
		if (cpu->preempt_disabled)
			return;

		if (cpu->thread && cpu->thread->state == THREAD_RUNNING)
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
