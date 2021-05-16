/* thread.c - x86 thread constructor */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/heap.h>
#include <kernel/thread.h>
#include <kernel/utils.h>
#include <arch/cpu.h>
#include <arch/interrupts.h>
#include <arch/selectors.h>
#include <arch/thread.h>

uint32_t struct_x86_thread_offsetof_esp = offsetof(struct x86_thread, esp);
static atomic_uint current_tid = 1;

static void set_name(struct x86_thread *thread, const char *name)
{
	size_t len = kstrlen(name);

	if (len > sizeof(thread->noarch.name))
		len = sizeof(thread->noarch.name) - 1;

	kstrncpy(thread->noarch.name, name, len);
	thread->noarch.name[len] = 0;
}

/* Allocates an empty thread object. This has only one purpose - to create the first kernel thread
   on the CPU. */
struct x86_thread *x86_thread_allocate_empty(struct x86_proc *proc, const char *name, uint16_t cs,
	uint16_t ds)
{
	struct x86_thread *thread = kalloc(HEAP_NORMAL, HEAP_NO_ALIGN, sizeof(struct x86_thread));

	/* We cast pointers here. Make sure we're not in the wrong. */
	kassert(sizeof(uint32_t) == sizeof(vaddr_t));

	set_name(thread, name);
	thread->noarch.tid = atomic_fetch_add(&current_tid, 1);
	thread->noarch.state = THREAD_NEW;

	/* Set the context. */
	thread->cs = cs;
	thread->ds = ds;
	thread->parent = proc;

	return thread;
}

struct x86_thread *x86_thread_allocate(struct x86_proc *proc, const char *name, uint16_t cs,
	uint16_t ds, vaddr_t stack, size_t stack_size, void (*tentry)(void), void (*entry)(void *),
	void *cookie)
{
	struct isr_frame *isr_frame;
	struct x86_switch_frame *switch_frame;
	struct x86_thread *thread;

	thread = x86_thread_allocate_empty(proc, name, cs, ds);

	thread->tentry = tentry;
	thread->entry = entry;
	thread->cookie = cookie;

	/* Create stacks. */
	if (cs != KERNEL_CODE_SELECTOR)
	{
		thread->stack0 = kalloc(HEAP_NORMAL, 16, KERNEL_STACK_SIZE);
		thread->ebp0 = (uint32_t)thread->stack0 + KERNEL_STACK_SIZE;
	}

	/* Make sure the stack is aligned to 16 bytes. */
	kassert(((uintptr_t)stack) % 16 == 0);

	thread->stack = stack;
	thread->ebp = (uint32_t)thread->stack + stack_size;
	thread->esp = thread->ebp;

	/* 2. We'll be switching in an interrupt handler. We need to imitate the stack during
	   an interrupt, so that iret works. */
	thread->esp = thread->esp - sizeof(struct isr_frame);
	isr_frame = (struct isr_frame *)thread->esp;

	isr_frame->cs = cs;
	isr_frame->eip = (uint32_t)tentry;

	isr_frame->ds = ds;
	isr_frame->es = ds;
	isr_frame->fs = 0;
	isr_frame->gs = 0;

	isr_frame->ss = ds;
	isr_frame->esp = thread->ebp;

	/* 1. After this thread is switched to, we will be in x86_thread_switch. Build it's frame once
	   the stack, but omitting the parameters. Make ret take us to isr_exit. */
	thread->esp = thread->esp - sizeof(struct x86_switch_frame);
	switch_frame = (struct x86_switch_frame *)thread->esp;

	switch_frame->ebp = thread->ebp;
	switch_frame->eip = (uint32_t)isr_exit;

	/* And finally, we're done! */
	return thread;
}

/* Frees the thread object. */
void x86_thread_free(struct x86_thread *thread)
{
	if (thread->stack0)
		kfree(thread->stack0);

	kfree(thread);
}
