/* arch/i386/syscall.c - x86 syscall entry point */
#include <kernel/syscall.h>
#include <arch/interrupts.h>
#include <arch/cpu/idt.h>

#define SYSCALL_EXIT 1

static void syscall_handler(struct isr_frame *frame)
{
	switch(frame->eax)
	{
	case SYSCALL_EXIT:
		syscall_exit();
	}
}

void init_syscall(void)
{
	/* Set the gate for the syscall interrupt to DPL=3 so that unprivileged code can use it. */
	idt_modify_entry(INT_SYSCALL, SEG_BIT_PRESENT | IDTE_INTERRUPT_32 | SEG_BIT_RING(3));

	/* Put the handler's address in the kernel's ISR registry. */
	isr_set_handler(INT_SYSCALL, syscall_handler);
}
