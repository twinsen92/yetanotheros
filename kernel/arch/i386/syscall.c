/* arch/i386/syscall.c - x86 syscall entry point */
#include <kernel/addr.h>
#include <kernel/proc.h>
#include <kernel/scheduler.h>
#include <kernel/syscall_impl.h>
#include <kernel/syscalls.h>
#include <arch/cpu.h>
#include <arch/interrupts.h>
#include <arch/paging.h>
#include <arch/cpu/idt.h>

static uint32_t syscall_brk(int func, uint32_t arg);

static void syscall_handler(struct isr_frame *frame)
{
	switch(frame->eax)
	{
	case SYSCALL_EXIT:
		syscall_exit();
	case SYSCALL_BRK:
		frame->eax = syscall_brk(SYSCALL_BRK, frame->ebx);
		break;
	case SYSCALL_SBRK:
		frame->eax = syscall_brk(SYSCALL_SBRK, frame->ebx);
		break;
	}
}

static uint32_t syscall_brk(int func, uint32_t arg)
{
	struct thread *thread;
	paddr_t cr3 = cpu_get_cr3();
	uint32_t ret;

	/* Don't want to get interrupted, and, possibly, preempted here. */
	kassert((cpu_get_eflags() & EFLAGS_IF) == 0);

	if (cr3 == phys_kernel_pd)
		kpanic("syscall_brk(): called with kernel page tables");

	/* We need to switch to kernel page tables to modify the process' virtual memory space. */
	cpu_set_cr3(phys_kernel_pd);

	thread = get_current_thread();

	if (thread == NULL)
		kpanic("syscall_brk(): null thread");

	if (thread->parent == NULL)
		kpanic("syscall_brk(): null proc");

	if (func == SYSCALL_BRK)
		ret = (uint32_t)proc_brk(thread->parent, (vaddr_t)arg);
	else if (func == SYSCALL_SBRK)
		ret = (uint32_t)proc_sbrk(thread->parent, (vaddrdiff_t)arg);
	else
		kpanic("syscall_brk(): bad function");

	cpu_set_cr3(cr3);

	return ret;
}

void init_syscall(void)
{
	/* Set the gate for the syscall interrupt to DPL=3 so that unprivileged code can use it. */
	idt_modify_entry(INT_SYSCALL, SEG_BIT_PRESENT | IDTE_INTERRUPT_32 | SEG_BIT_RING(3));

	/* Put the handler's address in the kernel's ISR registry. */
	isr_set_handler(INT_SYSCALL, syscall_handler);
}
