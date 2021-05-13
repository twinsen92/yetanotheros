/* interrupts.c - holder of interrupt handlers table */
#include <kernel/addr.h>
#include <kernel/debug.h>
#include <kernel/init.h>
#include <arch/cpu.h>
#include <arch/interrupts.h>

static void (*handlers[ISR_MAX])(isr_frame_t*);

/* Generic interrupt handler routine. */
void generic_interrupt_handler(isr_frame_t *frame)
{
	void (*handler)(isr_frame_t*);

	kassert(frame->int_no < ISR_MAX);
	handler = handlers[frame->int_no];

	if (handler != NULL)
		handler(frame);
	else
		kpanic("unhandled interrupt");
}

/* Initializes the ISR registry. */
void init_isr(void)
{
	kassert(is_yaos2_initialized() == false);

	for (int i = 0; i < ISR_MAX; i++)
		handlers[i] = NULL;
}

/* Sets a handler in the ISR registry. */
void isr_set_handler(int_no_t int_no, void (*handler)(isr_frame_t*))
{
	kassert(is_yaos2_initialized() == false);
	kassert(int_no < ISR_MAX);

	handlers[int_no] = handler;
}
