/* arch/i386/proc.c - x86 process management */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/heap.h>
#include <kernel/paging.h>
#include <kernel/proc.h>
#include <kernel/thread.h>
#include <kernel/utils.h>
#include <arch/memlayout.h>
#include <arch/palloc.h>
#include <arch/paging.h>
#include <arch/paging_types.h>
#include <arch/proc.h>

/* Creates a new proc object. */
struct proc *proc_alloc(const char *name)
{
	uint len;
	struct proc *proc;

	/* We need to read/write some physical pages. This has to be done with kernel page tables. */
	kassert(is_using_kernel_page_tables());

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
	proc->arch->vfrom = NULL;
	proc->arch->vto = NULL;
	proc->arch->vbreak = NULL;
	proc->arch->cur_vbreak = NULL;

	return proc;
}

/* Releases a given proc object and all related resources. */
void proc_free(struct proc *proc)
{
	vaddr_t v;
	pte_t pte;

	/* We need to read/write some physical pages. This has to be done with kernel page tables. */
	kassert(is_using_kernel_page_tables());

	/* We might call pfree(). */
	if (palloc_lock_held())
		kpanic("proc_vmreserve(): holding palloc lock");

	/* Free the physical pages allocated to this process. */
	v = proc->arch->vfrom;

	while (v <= proc->arch->vto)
	{
		pte = paging_get_entry(proc->arch->pd, v);

		/* Present, non-global pages have been allocated for this process. We can safely return
		   those pages to the physical page allocator. */
		if (pte & PAGE_BIT_PRESENT && (pte & PAGE_BIT_GLOBAL) == 0)
			pfree(pte_get_paddr(pte));

		v += PAGE_SIZE;
	}

	paging_free_dir(proc->arch->pd);
}

static void unsafe_vmreserve(struct proc *proc, vaddr_t v, uint flags)
{
	paddr_t p;
	pflags_t pflags;

	/* Get the virtual memory page. */
	v = (vaddr_t)mask_to_page(v);

	/* Check if we have to do anything. */
	p = paging_get(proc->arch->pd, v);

	if (p)
		return;

	/* Map flags to paging flags. */
	pflags = PAGE_BIT_PRESENT;

	if (flags & VM_USER)
		pflags |= PAGE_BIT_USER;

	if (flags & VM_WRITE)
		pflags |= PAGE_BIT_RW;

	/* Allocate a physical page and map it. */
	paging_map(proc->arch->pd, v, palloc(), pflags);

	/* Update VM pointers. */
	if (proc->arch->vfrom == NULL)
		proc->arch->vfrom = v;

	if (proc->arch->vto == NULL)
		proc->arch->vto= v;

	if (proc->arch->vfrom > v)
		proc->arch->vfrom = v;

	if (proc->arch->vto < v)
		proc->arch->vto = v;

	/* Notify other CPUs of the changes. */
	paging_propagate_changes(proc->arch->pd, v, false);
}

/* Reserves a physical page for the virtual memory page pointed at by v. */
void proc_vmreserve(struct proc *proc, vaddr_t v, uint flags)
{
	/* We need to read/write some physical pages. This has to be done with kernel page tables. */
	kassert(is_using_kernel_page_tables());

	/* We might call palloc(). */
	if (palloc_lock_held())
		kpanic("proc_vmreserve(): holding palloc lock");

	thread_mutex_acquire(&(proc->arch->pd_mutex));
	unsafe_vmreserve(proc, v, flags);
	thread_mutex_release(&(proc->arch->pd_mutex));
}

/* Write to the process' virtual memory. */
void proc_vmwrite(struct proc *proc, vaddr_t v, const void *buf, size_t num)
{
	/* We need to read/write some physical pages. This has to be done with kernel page tables. */
	kassert(is_using_kernel_page_tables());

	thread_mutex_acquire(&(proc->arch->pd_mutex));
	vmwrite(proc->arch->pd, v, buf, num);
	thread_mutex_release(&(proc->arch->pd_mutex));
}

/* Set the break pointer. */
void proc_set_break(struct proc *proc, vaddr_t v)
{
	thread_mutex_acquire(&(proc->arch->pd_mutex));
	proc->arch->vbreak = v;
	proc->arch->cur_vbreak = v;
	thread_mutex_release(&(proc->arch->pd_mutex));
}

static int unsafe_brk(struct proc *proc, vaddr_t v)
{
	vaddr_t vfrom;
	vaddr_t vto;

	/* Make sure we do not go lower than the base break address, and we do not place the new break
	   address in a kernel-occupied virtual memory region. */
	if (v < proc->arch->vbreak || vm_get_pflags(v) & PAGE_BIT_GLOBAL)
	{
		return -1;
	}

	/* Check if we can actually satisfy this call. */
	if (v - proc->arch->vbreak > (vaddrdiff_t)palloc_get_remaining())
	{
		return -1;
	}

	/* Reserve new pages. */
	vfrom = (vaddr_t)mask_to_page(proc->arch->cur_vbreak);
	vto = (vaddr_t)mask_to_page(v);

	while (vfrom <= vto)
	{
		unsafe_vmreserve(proc, vfrom, PAGE_BIT_USER | PAGE_BIT_RW);
		vfrom += PAGE_SIZE;
	}

	/* Update VM pointers. */
	proc->arch->cur_vbreak = v;

	if (proc->arch->vto < vto)
		proc->arch->vto = vto;

	return 0;
}

/* Changes the break pointer. Returns -1 on error. */
int proc_brk(struct proc *proc, vaddr_t v)
{
	int ret;

	/* We need to read/write some physical pages. This has to be done with kernel page tables. */
	kassert(is_using_kernel_page_tables());

	thread_mutex_acquire(&(proc->arch->pd_mutex));
	ret = unsafe_brk(proc, v);
	thread_mutex_release(&(proc->arch->pd_mutex));

	return ret;
}

/* Changes the break pointer. Returns -1 on error. */
vaddr_t proc_sbrk(struct proc *proc, vaddrdiff_t diff)
{
	vaddr_t ret = (vaddr_t)-1;

	/* We need to read/write some physical pages. This has to be done with kernel page tables. */
	kassert(is_using_kernel_page_tables());

	thread_mutex_acquire(&(proc->arch->pd_mutex));

	/* Try calling brk. After a successful call, cur_vbreak will be modified. */
	if (unsafe_brk(proc, proc->arch->cur_vbreak + diff) >= 0)
		ret = proc->arch->cur_vbreak;

	thread_mutex_release(&(proc->arch->pd_mutex));

	return ret;
}
