/* thread.c - x86 thread constructor */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/heap.h>
#include <kernel/proc.h>
#include <kernel/thread.h>
#include <kernel/utils.h>
#include <arch/cpu.h>
#include <arch/interrupts.h>
#include <arch/paging.h>
#include <arch/proc.h>
#include <arch/scheduler.h>
#include <arch/thread.h>
#include <arch/cpu/selectors.h>

uint32_t struct_x86_thread_offsetof_esp0 = offsetof(struct arch_thread, esp0);
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
	thread->collected_pid = 0;
	thread->collected_status = 0;
	thread->sleep_since = 0;
	thread->sleep_until = 0;
	thread->cond = NULL;
	thread->sched_count = 0;

	/* Set the context. */
	thread->arch->cs = cs;
	thread->arch->ds = ds;

	thread->arch->cr3 = phys_kernel_pd;
}

/* Builds a kernel x86_thread object. */
static void x86_thread_construct_thread(struct thread *thread,
	const char *name,
	vaddr_t stack0,
	size_t stack0_size,
	uvaddr_t stack,
	size_t stack_size,
	xvaddr_t tentry,
	void (*entry)(void *),
	void *cookie,
	bool int_enabled,
	uint16_t cs,
	uint16_t ds,
	paddr_t cr3)
{
	x86_thread_construct_empty(thread, name, cs, ds);

	thread->arch->tentry = tentry;
	thread->entry = entry;
	thread->cookie = cookie;

	/* Make sure the stack is aligned to 16 bytes. */
	kassert(((uintptr_t)stack) % 16 == 0);
	kassert(((uintptr_t)stack0) % 16 == 0);

	thread->arch->stack = UVNULL;
	thread->arch->stack_size = 0;
	thread->arch->ebp = 0;

	/* We need an N ring stack if we're creating a user thread. */
	if (cs == USER_CODE_SELECTOR)
	{
		kassert(stack);

		thread->arch->stack = stack;
		thread->arch->stack_size = stack_size;
		thread->arch->ebp = (uint32_t)thread->arch->stack + stack_size;
	}

	thread->arch->stack0 = stack0;
	thread->arch->stack0_size = stack0_size;
	thread->arch->ebp0 = (uint32_t)thread->arch->stack0 + stack0_size;
	thread->arch->esp0 = thread->arch->ebp0;

	thread->arch->int_enabled = int_enabled;
	thread->arch->cli_stack = 0;
	thread->arch->cr3 = cr3;
}

/* Setup for the initial kernel thread stack (starting at x86_thread_switch). */
static void setup_kthread_stack(struct thread *thread)
{
	struct isr_frame *isr_frame;
	struct x86_switch_frame *switch_frame;

	/* 2. We'll be switching in an interrupt handler. We need to imitate the stack during
	   an interrupt, so that iret works. */
	thread->arch->esp0 = thread->arch->esp0 - sizeof(struct isr_frame);
	isr_frame = (struct isr_frame *)thread->arch->esp0;

	/* isr_exit stack */
	isr_frame->ebp = thread->arch->ebp0;
	isr_frame->ds = thread->arch->ds;
	isr_frame->es = thread->arch->ds;
	isr_frame->fs = 0;
	isr_frame->gs = 0;

	/* IRET stack. No need to specify SS:ESP for kernel threads. */
	isr_frame->ss = 0;
	isr_frame->esp = 0;

	isr_frame->eflags = thread->arch->int_enabled ? EFLAGS_IF : 0;
	isr_frame->cs = thread->arch->cs;
	isr_frame->eip = (uint32_t)thread->arch->tentry;

	/* 1. After this thread is switched to, we will be in x86_thread_switch. Build it's frame once
	   the stack, but omitting the parameters. Make ret take us to isr_exit. */
	thread->arch->esp0 = thread->arch->esp0 - sizeof(struct x86_switch_frame);
	switch_frame = (struct x86_switch_frame *)thread->arch->esp0;

	switch_frame->ebp = thread->arch->ebp0;
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
	x86_thread_construct_thread(thread, name, stack, 4096, UVNULL, 0, (xvaddr_t)&kthread_entry, entry, cookie,
		true, KERNEL_CODE_SELECTOR, KERNEL_DATA_SELECTOR, phys_kernel_pd);
	setup_kthread_stack(thread);

	return thread;
}

/* Setup for the initial user thread stack (starting at x86_thread_switch). */
static void setup_uthread_stack(struct thread *thread, struct isr_frame *frame_template)
{
	struct isr_frame *isr_frame;
	struct x86_switch_frame *switch_frame;

	uint32_t *uptr;

	/* 3. We'll be switching in an interrupt handler. We need to imitate the stack during
	   an interrupt, so that iret works. */
	thread->arch->esp0 = thread->arch->esp0 - sizeof(struct isr_frame);
	isr_frame = (struct isr_frame *)thread->arch->esp0;

	if (!frame_template)
	{
		/* isr_exit stack */
		isr_frame->ebp = thread->arch->ebp0;
		isr_frame->ds = thread->arch->ds;
		isr_frame->es = thread->arch->ds;
		isr_frame->fs = 0;
		isr_frame->gs = 0;

		/* IRET stack. We're returning to a different ring, so we have to also specify SS:ESP. */
		isr_frame->ss = thread->arch->ds;
		isr_frame->esp = thread->arch->ebp;

		isr_frame->eflags = thread->arch->int_enabled ? EFLAGS_IF : 0;
		isr_frame->cs = thread->arch->cs;
		isr_frame->eip = (uint32_t)thread->arch->tentry;
	}
	else
	{
		kmemcpy(isr_frame, frame_template, sizeof(struct isr_frame));
	}

	/* 2. uthread_switch_entry stack. We just write a return address. */
	thread->arch->esp0 = thread->arch->esp0 - sizeof(uint32_t);
	uptr = (uint32_t *)thread->arch->esp0;
	*uptr = (uint32_t)isr_exit;

	/* 1. After this thread is switched to, we will be in x86_thread_switch. Build it's frame once
	   the stack, but omitting the parameters. Make ret take us to uthread_switch_entry. */
	thread->arch->esp0 = thread->arch->esp0 - sizeof(struct x86_switch_frame);
	switch_frame = (struct x86_switch_frame *)thread->arch->esp0;

	switch_frame->ebp = thread->arch->ebp0;
	switch_frame->eip = (uint32_t)uthread_switch_entry;
}

struct thread *uthread_create(uvaddr_t tentry, uvaddr_t stack, size_t stack_size,
	const char *name, struct proc *parent)
{
	struct thread *thread;
	vaddr_t stack0;

	stack0 = kalloc(HEAP_NORMAL, 16, 4096);
	thread = kalloc(HEAP_NORMAL, HEAP_NO_ALIGN, sizeof(struct thread));
	thread->arch = kalloc(HEAP_NORMAL, HEAP_NO_ALIGN, sizeof(struct arch_thread));
	x86_thread_construct_thread(thread, name, stack0, 4096, stack, stack_size, tentry, NULL, NULL,
		true, USER_CODE_SELECTOR, USER_DATA_SELECTOR, parent->arch->pd);
	setup_uthread_stack(thread, NULL);

	return thread;
}

struct thread *uthread_fork_create(struct proc *new_proc, struct thread *template, struct isr_frame *frame_template)
{
	struct isr_frame frame;
	struct thread *thread;
	vaddr_t stack0;

	/* Make sure we return 0 from the syscall on the forked process. */
	kmemcpy(&frame, frame_template, sizeof(struct isr_frame));
	frame.eax = 0;

	/*
	 * Create a thread that will return from the syscall interrupt handler. This thread will have
	 * the same state as the current thread.
	 */
	stack0 = kalloc(HEAP_NORMAL, 16, 4096);
	thread = kalloc(HEAP_NORMAL, HEAP_NO_ALIGN, sizeof(struct thread));
	thread->arch = kalloc(HEAP_NORMAL, HEAP_NO_ALIGN, sizeof(struct arch_thread));
	x86_thread_construct_thread(thread, template->name, stack0, 4096, new_proc->arch->vstack,
			new_proc->arch->stack_size, template->arch->tentry, NULL, NULL,
			true, USER_CODE_SELECTOR, USER_DATA_SELECTOR, new_proc->arch->pd);
	setup_uthread_stack(thread, &frame);

	return thread;
}

/* Frees a thread object. */
void thread_free(struct thread *thread)
{
#ifdef KERNEL_DEBUG
	if (thread->tid < MAX_THREADS)
		debug_x86_threads[thread->tid] = NULL;
#endif
	/* We do not free the N ring stack because it does not belong to thread.c. */
	kfree(thread->arch->stack0);
	kfree(thread->arch);
	kfree(thread);
}
