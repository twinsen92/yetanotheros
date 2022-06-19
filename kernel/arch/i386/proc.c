/* arch/i386/proc.c - x86 process management */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/heap.h>
#include <kernel/paging.h>
#include <kernel/proc.h>
#include <kernel/thread.h>
#include <kernel/utils.h>
#include <kernel/vfs.h>
#include <arch/cpu.h>
#include <arch/memlayout.h>
#include <arch/palloc.h>
#include <arch/paging.h>
#include <arch/paging_types.h>
#include <arch/proc.h>

#include <user/yaos2/kernel/errno.h>

/* Creates a new proc object. */
struct proc *proc_alloc(const char *name)
{
	uint len;
	struct proc *proc;

	/* We need to read/write some physical pages. This has to be done with kernel page tables. */
	kassert(is_using_kernel_page_tables());

	/* Allocate the object on the heap. */
	proc = kzualloc(HEAP_NORMAL, sizeof(struct proc));
	proc->arch = kzualloc(HEAP_NORMAL, sizeof(struct arch_proc));

	/* Copy the name. */
	len = kmin(kstrlen(name), sizeof(proc->name) - 1);
	kmemcpy(proc->name, name, len);
	proc->name[len] = 0;

	LIST_INIT(&(proc->threads));

	thread_mutex_create(&(proc->mutex));

	/* Create a page directory. */
	thread_mutex_create(&(proc->arch->pd_mutex));
	proc->arch->pd = paging_alloc_dir();
	proc->arch->vfrom = UVNULL;
	proc->arch->vto = UVNULL;
	proc->arch->vbreak = UVNULL;
	proc->arch->cur_vbreak = UVNULL;

	return proc;
}

/* Releases a given proc object and all related resources. */
void proc_free(struct proc *proc)
{
	uvaddr_t v;
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
		if ((pte & PAGE_BIT_PRESENT) && (pte & PAGE_BIT_GLOBAL) == 0)
			pfree(pte_get_paddr(pte));

		v += PAGE_SIZE;
	}

	paging_free_dir(proc->arch->pd);
}

static void unsafe_vmreserve(struct proc *proc, uvaddr_t v, uint flags)
{
	paddr_t p;
	pflags_t pflags;

	/* Get the virtual memory page. */
	v = (uvaddr_t)mask_to_page(v);

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
	if (proc->arch->vfrom == UVNULL)
		proc->arch->vfrom = v;

	if (proc->arch->vto == UVNULL)
		proc->arch->vto= v;

	if (proc->arch->vfrom > v)
		proc->arch->vfrom = v;

	if (proc->arch->vto < v)
		proc->arch->vto = v;

	/* Notify other CPUs of the changes. */
	paging_propagate_changes(proc->arch->pd, v, false);
}

/* Reserves a physical page for the virtual memory page pointed at by v. */
void proc_vmreserve(struct proc *proc, uvaddr_t v, uint flags)
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

/* Read from the process' virtual memory. TODO: Check the address and return some information. */
void proc_vmread(struct proc *proc, uvaddr_t v, void *buf, size_t num)
{
	/* We need to read/write some physical pages. This has to be done with kernel page tables. */
	kassert(is_using_kernel_page_tables());

	thread_mutex_acquire(&(proc->arch->pd_mutex));
	vmread(proc->arch->pd, v, buf, num);
	thread_mutex_release(&(proc->arch->pd_mutex));
}

/* Write to the process' virtual memory. TODO: Check the address and return some information. */
void proc_vmwrite(struct proc *proc, uvaddr_t v, const void *buf, size_t num)
{
	/* We need to read/write some physical pages. This has to be done with kernel page tables. */
	kassert(is_using_kernel_page_tables());

	thread_mutex_acquire(&(proc->arch->pd_mutex));
	vmwrite(proc->arch->pd, v, buf, num);
	thread_mutex_release(&(proc->arch->pd_mutex));
}

/* Set the break pointer. */
void proc_set_break(struct proc *proc, uvaddr_t v)
{
	thread_mutex_acquire(&(proc->arch->pd_mutex));
	proc->arch->vbreak = v;
	proc->arch->cur_vbreak = v;
	thread_mutex_release(&(proc->arch->pd_mutex));
}

/* Set process virtual memory. */
void proc_set_uvm(struct proc *proc)
{
	cpu_set_cr3(proc->arch->pd);
}

/* Set kernel virtual memory. */
void proc_set_kvm(void)
{
	cpu_set_cr3(phys_kernel_pd);
}

static int unsafe_brk(struct proc *proc, uvaddr_t v)
{
	uvaddr_t vfrom;
	uvaddr_t vto;

	/* Make sure we do not go lower than the base break address, and we do not place the new break
	   address in a kernel-occupied virtual memory region. */
	if (v < proc->arch->vbreak || (vm_get_pflags((vaddr_t)v) & PAGE_BIT_GLOBAL))
	{
		return -EUNSPEC;
	}

	/* Check if we can actually satisfy this call. */
	if (v - proc->arch->vbreak > palloc_get_remaining())
	{
		return -EUNSPEC;
	}

	/* Reserve new pages. */
	vfrom = (uvaddr_t)mask_to_page(proc->arch->cur_vbreak);
	vto = (uvaddr_t)mask_to_page(v);

	while (vfrom <= vto)
	{
		unsafe_vmreserve(proc, vfrom, VM_USER | VM_WRITE);
		vfrom += PAGE_SIZE;
	}

	/* Update VM pointers. */
	proc->arch->cur_vbreak = v;

	if (proc->arch->vto < vto)
		proc->arch->vto = vto;

	return 0;
}

/* Changes the break pointer. Returns -1 on error. */
int proc_brk(struct proc *proc, uvaddr_t v)
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
uvaddr_t proc_sbrk(struct proc *proc, uvaddrdiff_t diff)
{
	uvaddr_t ret;
	int result;

	/* We need to read/write some physical pages. This has to be done with kernel page tables. */
	kassert(is_using_kernel_page_tables());

	thread_mutex_acquire(&(proc->arch->pd_mutex));

	/* Remember the previous break address. Call brk and return -1 on error. */
	ret = proc->arch->cur_vbreak;

	result = unsafe_brk(proc, proc->arch->cur_vbreak + diff);

	if (result < 0)
		ret = (uvaddr_t)result;

	thread_mutex_release(&(proc->arch->pd_mutex));

	return ret;
}
