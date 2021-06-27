/* arch/thread.h - x86 threading structures and constructors */
#ifndef ARCH_I386_THREAD_H
#define ARCH_I386_THREAD_H

#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/proc.h>
#include <kernel/thread.h>

/* Size of the kernel stack if one has to be allocated. */
#define KERNEL_STACK_SIZE 4096

struct arch_thread
{
	uint16_t cs; /* Code selector. */
	uint16_t ds; /* Data selector. */

	/* Ring N stack. */
	uint32_t esp; /* Current stack pointer. */
	uint32_t ebp; /* Bottom of the stack pointer. */
	vaddr_t stack; /* Stack top pointer. */
	size_t stack_size;

	/* Ring 0 stack. */
	uint32_t ebp0; /* Bottom of the kernel stack pointer. 0 if using kernel selectors. */
	vaddr_t stack0; /* Kernel stack top pointer. NULL if using kernel selectors. */
	size_t stack0_size;

	/* Interrupts state. These should be filled in by the scheduler. */
	bool int_enabled; /* Interrupts state when cli_stack was 0. */
	int cli_stack; /* Number of cli push operations. */

	void (*tentry)(void); /* Entry point of the thread. This is what IRET will take the CPU to. */
};

/* Builds an empty thread object. This has only one purpose - to create the first kernel thread
   on the CPU. */
void x86_thread_construct_empty(struct thread *thread, struct proc *proc, const char *name,
	uint16_t cs, uint16_t ds);

/* Builds a kernel thread object. */
void x86_thread_construct_kthread(struct thread *thread, struct proc *proc,
	const char *name, vaddr_t stack, size_t stack_size, void (*tentry)(void), void (*entry)(void *),
	void *cookie, bool int_enabled);

/* This is what the stack looks like when x86_thread_switch switches stack pointers. Parameters,
   return address and saved EBP are omitted. */
packed_struct x86_switch_frame
{
	uint32_t edi; /*  0: Saved %edi. */
	uint32_t esi; /*  4: Saved %esi. */
	uint32_t ebp; /*  8: Saved %ebp. */
	uint32_t ebx; /* 12: Saved %ebx. */
	uint32_t eip; /* 16: Return address. */
};

/* Switches threads by switching stack pointers. */
void x86_thread_switch(struct arch_thread *current, struct arch_thread *new);

#endif
