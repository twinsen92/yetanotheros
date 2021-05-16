/* thread.h - x86 threading structures and constructors */
#ifndef ARCH_I386_THREAD_H
#define ARCH_I386_THREAD_H

#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/queue.h>
#include <kernel/thread.h>

/* Size of the kernel stack if one has to be allocated. */
#define KERNEL_STACK_SIZE 4096

struct x86_proc;

struct x86_thread
{
	uint16_t cs; /* Code selector. */
	uint16_t ds; /* Data selector. */

	/* Ring N stack. */
	uint32_t esp; /* Current stack pointer. */
	uint32_t ebp; /* Bottom of the stack pointer. */
	vaddr_t stack; /* Stack top pointer. */

	/* Ring 0 stack. */
	uint32_t ebp0; /* Bottom of the kernel stack pointer. 0 if using kernel selectors. */
	vaddr_t stack0; /* Kernel stack top pointer. NULL if using kernel selectors. */

	void (*tentry)(void); /* Entry point of the thread. This is what IRET will take the CPU to. */
	void (*entry)(void *); /* Entry point the scheduler will call. */
	void *cookie; /* The scheduler will pass this cookie to the entry point. */

	struct x86_proc *parent; /* Parent process */
	struct thread noarch; /* Arch-independent structure. */

	LIST_ENTRY(x86_thread) lptrs;
	STAILQ_ENTRY(x86_thread) sqptrs;
};

LIST_HEAD(x86_thread_list, x86_thread);

/* Allocates an empty thread object. This has only one purpose - to create the first kernel thread
   on the CPU. */
struct x86_thread *x86_thread_allocate_empty(struct x86_proc *proc, const char *name, uint16_t cs,
	uint16_t ds);

/* Allocates a thread object.  */
struct x86_thread *x86_thread_allocate(struct x86_proc *proc, const char *name, uint16_t cs,
	uint16_t ds, vaddr_t stack, size_t stack_size, void (*tentry)(void), void (*entry)(void *),
	void *cookie);

/* Frees the thread object. */
void x86_thread_free(struct x86_thread *thread);

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
void x86_thread_switch(struct x86_thread *current, struct x86_thread *new);

#endif
