/* arch/i386/syscall.c - x86 syscall entry point */
#include <kernel/addr.h>
#include <kernel/syscall_impl.h>
#include <arch/cpu.h>
#include <arch/interrupts.h>
#include <arch/paging.h>
#include <arch/cpu/idt.h>

#include <user/yaos2/kernel/syscalls.h>

static void syscall_handler(struct isr_frame *frame)
{
	switch(frame->eax)
	{
	case SYSCALL_EXIT:
		syscall_exit(frame->ebx);

	case SYSCALL_BRK:
		frame->eax = (uint32_t)syscall_brk((uvaddr_t)frame->ebx);
		break;
	case SYSCALL_SBRK:
		frame->eax = (uint32_t)syscall_sbrk((uvaddrdiff_t)frame->ebx);
		break;

	case SYSCALL_OPEN:
		frame->eax = (uint32_t)syscall_open((uvaddr_t)frame->ebx, (int)frame->ecx);
		break;
	case SYSCALL_CLOSE:
		frame->eax = (uint32_t)syscall_close((int)frame->ebx);
		break;
	case SYSCALL_READ:
		frame->eax = (uint32_t)syscall_read((int)frame->ebx, (uvaddr_t)frame->ecx, (size_t)frame->edx);
		break;
	case SYSCALL_WRITE:
		frame->eax = (uint32_t)syscall_write((int)frame->ebx, (uvaddr_t)frame->ecx, (size_t)frame->edx);
		break;
	}
}

void init_syscall(void)
{
	/* Set the gate for the syscall interrupt to DPL=3 so that unprivileged code can use it. */
	idt_modify_entry(INT_SYSCALL, SEG_BIT_PRESENT | IDTE_TRAP_32 | SEG_BIT_RING(3));

	/* Put the handler's address in the kernel's ISR registry. */
	isr_set_handler(INT_SYSCALL, syscall_handler);
}
