/* thread.c - x86 thread constructor */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/heap.h>
#include <kernel/thread.h>
#include <kernel/utils.h>
#include <arch/cpu.h>
#include <arch/interrupts.h>
#include <arch/scheduler.h>
#include <arch/thread.h>
#include <arch/cpu/selectors.h>

uint32_t struct_x86_thread_offsetof_esp = offsetof(struct arch_thread, esp);
static atomic_uint current_thread_no = 1;

#ifdef KERNEL_DEBUG
#define MAX_THREADS 128
/* An array containing up to MAX_THREADS pointers to thread structures. This is here only to ease
   debugging of memory dumps. */
struct thread *debug_x86_threads[MAX_THREADS];
#endif

static void set_name(struct thread *thread, const char *name)
{
	size_t len = kstrlen(name);

	if (len > sizeof(thread->name))
		len = sizeof(thread->name) - 1;

	kstrncpy(thread->name, name, len);
	thread->name[len] = 0;
}

/* Builds an empty x86_thread object. This has only one purpose - to create the first kernel thread
   on the CPU. */
void x86_thread_construct_empty(struct thread *thread, const char *name, uint16_t cs, uint16_t ds)
{
	/* We cast pointers here. Make sure we're not in the wrong. */
	kassert(sizeof(uint32_t) == sizeof(vaddr_t));

	set_name(thread, name);

#ifdef KERNEL_DEBUG
	uint thread_no = atomic_fetch_add(&current_thread_no, 1);
	if (thread_no < MAX_THREADS)
		debug_x86_threads[thread_no] = thread;
#endif

	thread->state = THREAD_NEW;
	thread->sleep_since = 0;
	thread->sleep_until = 0;
	thread->cond = NULL;

	/* Set the context. */
	thread->arch->cs = cs;
	thread->arch->ds = ds;
}

/* Builds a kernel x86_thread object. */
void x86_thread_construct_kthread(struct thread *thread, const char *name, vaddr_t stack,
	size_t stack_size, void (*tentry)(void), void (*entry)(void *), void *cookie, bool int_enabled)
{
	struct isr_frame *isr_frame;
	struct x86_switch_frame *switch_frame;

	x86_thread_construct_empty(thread, name, KERNEL_CODE_SELECTOR, KERNEL_DATA_SELECTOR);

	thread->arch->tentry = tentry;
	thread->entry = entry;
	thread->cookie = cookie;

	/* Make sure the stack is aligned to 16 bytes. */
	kassert(((uintptr_t)stack) % 16 == 0);

	thread->arch->stack = stack;
	thread->arch->stack_size = stack_size;
	thread->arch->ebp = (uint32_t)thread->arch->stack + stack_size;
	thread->arch->esp = thread->arch->ebp;

	/* Don't need to set up a ring 0 stack for a kernel thread. */
	thread->arch->stack0 = NULL;
	thread->arch->stack0_size = 0;
	thread->arch->ebp0 = 0;

	thread->arch->int_enabled = int_enabled;
	thread->arch->cli_stack = 0;

	/* 2. We'll be switching in an interrupt handler. We need to imitate the stack during
	   an interrupt, so that iret works. */
	thread->arch->esp = thread->arch->esp - sizeof(struct isr_frame);
	isr_frame = (struct isr_frame *)thread->arch->esp;

	/* isr_exit stack */
	isr_frame->ebp = thread->arch->ebp;
	isr_frame->ds = KERNEL_DATA_SELECTOR;
	isr_frame->es = KERNEL_DATA_SELECTOR;
	isr_frame->fs = 0;
	isr_frame->gs = 0;

	/* IRET stack. Since we're IRETing to the same ring, no need to provide SS:ESP */
	isr_frame->ss = 0;
	isr_frame->esp = 0;
	isr_frame->eflags = int_enabled ? EFLAGS_IF : 0;
	isr_frame->cs = KERNEL_CODE_SELECTOR;
	isr_frame->eip = (uint32_t)tentry;

	/* 1. After this thread is switched to, we will be in x86_thread_switch. Build it's frame once
	   the stack, but omitting the parameters. Make ret take us to isr_exit. */
	thread->arch->esp = thread->arch->esp - sizeof(struct x86_switch_frame);
	switch_frame = (struct x86_switch_frame *)thread->arch->esp;

	switch_frame->ebp = thread->arch->ebp;
	switch_frame->eip = (uint32_t)isr_exit;
}

/* Creates a kernel thread. */
struct thread *kthread_create(void (*entry)(void *), void *cookie, const char *name)
{
	struct thread *thread;
	vaddr_t stack;

	stack = kalloc(HEAP_NORMAL, 16, 4096);
	thread = kalloc(HEAP_NORMAL, HEAP_NO_ALIGN, sizeof(struct thread));
	thread->arch = kalloc(HEAP_NORMAL, HEAP_NO_ALIGN, sizeof(struct arch_thread));
	x86_thread_construct_kthread(thread, name, stack, 4096, &thread_entry, entry, cookie, true);

	return thread;
}

/* Frees a thread object. */
void thread_free(struct thread *thread)
{
	kfree(thread->arch->stack);
	if (thread->arch->stack0)
		kfree(thread->arch->stack0);
	kfree(thread->arch);
	kfree(thread);
}
