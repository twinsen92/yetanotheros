/* interrupts.c - holder of interrupt handlers table */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/init.h>
#include <kernel/interrupts.h>
#include <kernel/scheduler.h>
#include <kernel/thread.h>
#include <arch/cpu.h>
#include <arch/interrupts.h>
#include <arch/paging.h>
#include <arch/scheduler.h>
#include <arch/selectors.h>

static void (*handlers[ISR_MAX])(isr_frame_t*);

/* Generic interrupt handler routine. */
void generic_interrupt_handler(isr_frame_t *frame)
{
	void (*handler)(isr_frame_t*);

	kassert(frame->int_no < ISR_MAX);
	handler = handlers[frame->int_no];

	/* When kernel page tables are updated (for example in heap.c) other CPUs might not be aware of
	   the changes. We cannot execute invlpg on all CPUs, but we can and do set kvm_changed in each
	   CPU object. If that flag is set, it means we have to flush the TLB and all should be OK. */
	if (frame->int_no == INT_PAGEFAULT && frame->cs == KERNEL_CODE_SELECTOR)
	{
		push_no_interrupts();

		bool kvm_changed = atomic_exchange(&(cpu_current()->kvm_changed), false);

		if (kvm_changed)
			cpu_flush_tlb();

		pop_no_interrupts();
		return;
	}

	if (handler != NULL)
		handler(frame);
	else
		kpanic("unhandled interrupt");


	// Force process to give up CPU on clock tick.
	// If interrupts were on while locks held, would need to check nlock.
	if(get_current_thread() && get_current_thread()->noarch.state == THREAD_RUNNING &&
		frame->int_no == INT_IRQ_TIMER)
		thread_yield();
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
