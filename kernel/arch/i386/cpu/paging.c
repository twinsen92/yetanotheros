/* cpu/paging.c - memory paging subsystem */
#include <kernel/addr.h>
#include <kernel/cpu.h>
#include <kernel/debug.h>
#include <arch/cpu.h>
#include <arch/palloc.h>
#include <arch/memlayout.h>
#include <arch/cpu/paging.h>

pde_t *const current_pd = (pde_t *const) 0xfffff000;
pte_t *const current_page_tables = (pte_t *const) 0xffc00000;
pde_t *const kernel_pd = get_symbol_vaddr(__kernel_pd);
pte_t *const kernel_page_tables = get_symbol_vaddr(__kernel_page_tables);

/* Kernel page tables lock. Acquire this to modify kernel page tables. */
static struct cpu_spinlock kp_spinlock;

static vaddr_t unsafe_translate_or_panic(paddr_t p)
{
	/* We assume we can do a simple translation of the physical address using the virtual memory
	   map. This is because with kernel pages, we can reach all of the memory allocated by
	   the physical memory allocator, at any time. */

	kassert(is_using_kernel_page_tables());

	if (vm_is_static_p(p))
		return vm_map_walk(p, true);

	kpanic("could not translate address");
}

static void unsafe_map(pde_t *const pd, vaddr_t v, paddr_t p, pflags_t flags)
{
	int pdi, pti;
	pde_t *pde;
	pte_t *pte;

	/* We do not want to (for whatever, future reason) be holding the physical memory allocator's
	   (palloc.c) lock right now... That could mean a dead-lock */
	if (palloc_lock_held ())
		kpanic("holding palloc lock while page mapping");

	/* Mapping should only be done with kernel page tables. */
	kassert(is_using_kernel_page_tables());

	v = (vaddr_t)mask_to_page(v);

	pdi = get_pd_index(v);
	pti = get_pt_index(v);

	/* Make sure the PD entry points at our PT. */
	pde = pd + pdi;

	if (pde_get_paddr(*pde) == PHYS_NULL)
	{
		*pde = pde_construct(palloc(), 0);
	}

	*pde |= flags | PAGE_BIT_PRESENT;

	/* Modify the PT. */
	pte = unsafe_translate_or_panic(pde_get_paddr(*pde));
	pte = pte + pti;

	*pte = pte_construct(p, flags | PAGE_BIT_PRESENT);
}

/* Initializes the interface of paging.h */
void init_paging(void)
{
	cpu_spinlock_create(&kp_spinlock, "kernel page tables write");
	init_paging_ipi();
}

/* Check if we're holding the kernel page table's lock. This is used to avoid dead-locks. */
bool kp_lock_held(void)
{
	return cpu_spinlock_held(&kp_spinlock);
}

/* Map one physical page to one virtual page. */
void kp_map(vaddr_t v, paddr_t p)
{
	paddr_t p_kernel_pd = vm_map_rev_walk(kernel_pd, true);
	paddr_t prev_cr3;
	pflags_t flags = vm_get_pflags(v);

	cpu_spinlock_acquire(&kp_spinlock);

	/* We need to use the kernel page tables, because we might call palloc to allocate a page
	   table. Also we might need to be able to access pages that might be beneath user's virtual
	   memory space. */
	prev_cr3 = cpu_set_cr3(p_kernel_pd);
	unsafe_map(kernel_pd, v, p, flags);
	/* Notify other CPUs of these changes. */
	paging_propagate_changes(p_kernel_pd, v, flags & PAGE_BIT_GLOBAL);
	/* If we think we're not going to switch CR3, we should do INVLPG */
	if (prev_cr3 == p_kernel_pd)
		asm_invlpg(v);
	cpu_set_cr3(prev_cr3);

	cpu_spinlock_release(&kp_spinlock);
}
