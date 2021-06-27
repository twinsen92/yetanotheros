/* arch/i386/paging/core.c - core x86 utilities for creating and editing page tables */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/paging.h>
#include <kernel/utils.h>
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

static inline void check_palloc_lock(void)
{
	if (palloc_lock_held())
		kpanic("paging_map(): holding palloc lock");
}

/* Allocate a new PD. */
paddr_t paging_alloc_dir(void)
{
	paddr_t pd;
	pde_t *vpd;

	/* We need to write to a physical page. This has to be done with kernel page tables. */
	kassert(is_using_kernel_page_tables());

	/* Potential deadlock because of our palloc() use. */
	check_palloc_lock();

	/* We assume page returned by palloc equals the size of page directory. */
	kassert(PD_LENGTH * sizeof(pde_t) == palloc_get_granularity());

	/* Allocate and clear the page directory. */
	pd = palloc();
	vpd = ptranslate(pd);
	kmemset(vpd, 0, PD_LENGTH * sizeof(pde_t));

	return pd;
}

/* Free a PD. */
void paging_free_dir(__unused paddr_t pd)
{
	/* Need to be using kernel pages. */
	kassert(is_using_kernel_page_tables());

	/* Potential deadlock because of our pfree() use. */
	check_palloc_lock();

	/* TODO: Implement */
	kpanic("not implemented");
}

/* Map physical page p to virtual address v using given flags for page tables in pd. */
void paging_map(paddr_t pd, vaddr_t v, paddr_t p, pflags_t flags)
{
	int pdi, pti;
	pde_t *pde;
	pte_t *pte;
	paddr_t pt;
	pte_t *vpt;

	/* Potential deadlock because of our palloc() use. */
	check_palloc_lock();

	/* We need to read and write some physical pages. This has to be done with kernel page tables. */
	kassert(is_using_kernel_page_tables());

	v = (vaddr_t)mask_to_page(v);

	pdi = get_pd_index(v);
	pti = get_pt_index(v);

	/* Make sure the PD entry points at our PT. */
	pde = translate_or_panic(pd);
	pde = pde + pdi;

	if (pde_get_paddr(*pde) == PHYS_NULL)
	{
		/* We assume page returned by palloc equals the size of a page table. */
		kassert(PT_LENGTH * sizeof(pte_t) == palloc_get_granularity());

		/* Allocate the page table and clear it. */
		pt = palloc();
		vpt = ptranslate(pt);
		kmemset(vpt, 0, PT_LENGTH * sizeof(pte_t));

		/* Write it to the page directory. */
		*pde = pde_construct(pt, 0);
	}

	*pde |= flags | PAGE_BIT_PRESENT;

	/* Modify the PT. */
	pte = translate_or_panic(pde_get_paddr(*pde));
	pte = pte + pti;

	*pte = pte_construct(p, flags | PAGE_BIT_PRESENT);
}

/* Get the physical address of the virutal address v in page tables pd. */
paddr_t paging_get(paddr_t pd, vaddr_t v)
{
	int pdi, pti;
	pde_t *pde;
	pte_t *pte;

	/* We need to read some physical pages. This has to be done with kernel page tables. */
	kassert(is_using_kernel_page_tables());

	v = (vaddr_t)mask_to_page(v);

	pdi = get_pd_index(v);
	pti = get_pt_index(v);

	/* Translate the first level. */
	pde = translate_or_panic(pd);
	pde = pde + pdi;

	/* Translate the second level. */
	pte = translate_or_panic(pde_get_paddr(*pde));
	pte = pte + pti;

	/* We can now read the page table. */
	return pte_get_paddr(*pte);
}

/* Write num number of bytes from buf into the virtual address v of page tables pd. */
void vmwrite(paddr_t pd, vaddr_t v, const void *buf, size_t num)
{
	paddr_t p;
	byte *writable;
	size_t off, batch;

	/* Need to be using kernel pages. */
	kassert(is_using_kernel_page_tables());

	while (num > 0)
	{
		/* Get the page from the page tables and translate it to a writable address. */
		p = paging_get(pd, v);
		writable = translate_or_panic(p);

		/* Calculate how much we have to write to this page. */
		off = (size_t)(v - mask_to_page(v));
		batch = kmin(PAGE_SIZE - off, num);

		/* Write the bytes. */
		kmemcpy(writable + off, buf, batch);
		num -= batch;

		/* Move onto the next page. */
		v = (vaddr_t)(mask_to_page(v) + PAGE_SIZE);
	}
}
