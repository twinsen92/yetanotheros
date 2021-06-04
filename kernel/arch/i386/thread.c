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

#ifdef KERNEL_DEBUG
#define MAX_THREADS 128
/* An array containing up to MAX_THREADS pointers to thread structures. This is here only to ease
   debugging of memory dumps. */
struct x86_thread *debug_x86_threads[MAX_THREADS];
#endif

static void set_name(struct x86_thread *thread, const char *name)
{
	size_t len = kstrlen(name);

	if (len > sizeof(thread->noarch.name))
		len = sizeof(thread->noarch.name) - 1;

	kstrncpy(thread->noarch.name, name, len);
	thread->noarch.name[len] = 0;
}

/* Builds an empty x86_thread object. This has only one purpose - to create the first kernel thread
   on the CPU. */
void x86_thread_construct_empty(struct x86_thread *thread, struct x86_proc *proc, const char *name,
	uint16_t cs, uint16_t ds)
{
	/* We cast pointers here. Make sure we're not in the wrong. */
	kassert(sizeof(uint32_t) == sizeof(vaddr_t));

	set_name(thread, name);
	thread->noarch.tid = atomic_fetch_add(&current_tid, 1);

#ifdef KERNEL_DEBUG
	if (thread->noarch.tid < MAX_THREADS)
		debug_x86_threads[thread->noarch.tid] = thread;
#endif

	thread->noarch.state = THREAD_NEW;
	thread->noarch.sleep_since = 0;
	thread->noarch.sleep_until = 0;
	thread->noarch.cond = NULL;

	/* Set the context. */
	thread->cs = cs;
	thread->ds = ds;
	thread->parent = proc;
}

/* Builds a kernel x86_thread object. */
void x86_thread_construct_kthread(struct x86_thread *thread, struct x86_proc *proc,
	const char *name, vaddr_t stack, size_t stack_size, void (*tentry)(void), void (*entry)(void *),
	void *cookie)
{
	struct isr_frame *isr_frame;
	struct x86_switch_frame *switch_frame;

	x86_thread_construct_empty(thread, proc, name, KERNEL_CODE_SELECTOR, KERNEL_DATA_SELECTOR);

	thread->tentry = tentry;
	thread->entry = entry;
	thread->cookie = cookie;

	/* Make sure the stack is aligned to 16 bytes. */
	kassert(((uintptr_t)stack) % 16 == 0);

	thread->stack = stack;
	thread->stack_size = stack_size;
	thread->ebp = (uint32_t)thread->stack + stack_size;
	thread->esp = thread->ebp;

	/* Don't need to set up a ring 0 stack for a kernel thread. */
	thread->stack0 = NULL;
	thread->stack0_size = 0;
	thread->ebp0 = 0;

	/* 2. We'll be switching in an interrupt handler. We need to imitate the stack during
	   an interrupt, so that iret works. */
	thread->esp = thread->esp - sizeof(struct isr_frame);
	isr_frame = (struct isr_frame *)thread->esp;

	/* isr_exit stack */
	isr_frame->ebp = thread->ebp;
	isr_frame->ds = KERNEL_DATA_SELECTOR;
	isr_frame->es = KERNEL_DATA_SELECTOR;
	isr_frame->fs = 0;
	isr_frame->gs = 0;

	/* IRET stack. Since we're IRETing to the same ring, no need to provide SS:ESP */
	isr_frame->ss = 0;
	isr_frame->esp = 0;
	isr_frame->eflags = 0;
	isr_frame->cs = KERNEL_CODE_SELECTOR;
	isr_frame->eip = (uint32_t)tentry;

	/* 1. After this thread is switched to, we will be in x86_thread_switch. Build it's frame once
	   the stack, but omitting the parameters. Make ret take us to isr_exit. */
	thread->esp = thread->esp - sizeof(struct x86_switch_frame);
	switch_frame = (struct x86_switch_frame *)thread->esp;

	switch_frame->ebp = thread->ebp;
	switch_frame->eip = (uint32_t)isr_exit;
}
