/* arch/i386/paging/core.c - core x86 utilities for creating and editing page tables */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <arch/memlayout.h>
#include <arch/paging.h>
#include <arch/paging_types.h>
#include <arch/palloc.h>

static vaddr_t translate_or_panic(paddr_t p)
{
	/* We assume we can do a simple translation of the physical address using the virtual memory
	   map. This is because with kernel pages, we can reach all of the memory allocated by
	   the physical memory allocator, at any time. */

	kassert(is_using_kernel_page_tables());

	if (vm_is_static_p(p))
		return vm_map_walk(p, true);

	kpanic("could not translate address");
}

/* Map physical page p to virtual address v using given flags for page tables in pd. */
void paging_map(pde_t *pd, vaddr_t v, paddr_t p, pflags_t flags)
{
	int pdi, pti;
	pde_t *pde;
	pte_t *pte;

	/* We do not want to (for whatever, future reason) be holding the physical memory allocator's
	   (palloc.c) lock right now... That could mean a dead-lock */
	if (palloc_lock_held())
		kpanic("paging_map(): holding palloc lock");

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
	pte = translate_or_panic(pde_get_paddr(*pde));
	pte = pte + pti;

	*pte = pte_construct(p, flags | PAGE_BIT_PRESENT);
}
