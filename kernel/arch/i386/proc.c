/* arch/i386/proc.c - x86 process management */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/heap.h>
#include <kernel/paging.h>
#include <kernel/proc.h>
#include <kernel/thread.h>
#include <kernel/utils.h>
#include <arch/palloc.h>
#include <arch/paging.h>
#include <arch/paging_types.h>
#include <arch/proc.h>

/* Creates a new proc object. */
struct proc *proc_alloc(const char *name)
{
	uint len;
	struct proc *proc;

	/* Allocate the object on the heap. */
	proc = kalloc(HEAP_NORMAL, 1, sizeof(struct proc));
	proc->arch = kalloc(HEAP_NORMAL, 1, sizeof(struct arch_proc));

	/* Copy the name. */
	len = kmin(kstrlen(name), sizeof(proc->name) - 1);
	kmemcpy(proc->name, name, len);
	proc->name[len] = 0;

	/* Create a page directory. */
	thread_mutex_create(&(proc->arch->pd_mutex));
	proc->arch->pd = paging_alloc_dir();

	return proc;
}

/* Releases a given proc object and all related resources. */
void proc_free(__unused struct proc *proc)
{
	kpanic("not implemented");
}

/* Reserves a physical page for the virtual memory page pointed at by v. */
void proc_vmreserve(struct proc *proc, vaddr_t v, uint flags)
{
	paddr_t p;
	pflags_t pflags;

	/* We might call palloc(). */
	if (palloc_lock_held())
		kpanic("proc_vmreserve(): holding palloc lock");

	/* Get the virtual memory page. */
	v = (vaddr_t)mask_to_page(v);

	thread_mutex_acquire(&(proc->arch->pd_mutex));

	/* Check if we have to do anything. */
	p = paging_get(proc->arch->pd, v);

	if (p)
		goto done;

	/* Map flags to paging flags. */
	pflags = PAGE_BIT_PRESENT;

	if (flags & VM_USER)
		pflags |= PAGE_BIT_USER;

	if (flags & VM_WRITE)
		pflags |= PAGE_BIT_RW;

	/* Allocate a physical page and map it. */
	paging_map(proc->arch->pd, v, palloc(), pflags);

done:
	thread_mutex_release(&(proc->arch->pd_mutex));
}

/* Write to the process' virtual memory. */
void proc_vmwrite(struct proc *proc, vaddr_t v, const void *buf, size_t num)
{
	thread_mutex_acquire(&(proc->arch->pd_mutex));
	vmwrite(proc->arch->pd, v, buf, num);
	thread_mutex_release(&(proc->arch->pd_mutex));
}
