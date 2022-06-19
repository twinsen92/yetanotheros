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

	/* Copy the global entries from the kernel page directory. The kernel page directory will always
	   be up to date, because we set all entries to present, with some also being global. */

	kp_lock();

	for (uint i = 0; i < PD_LENGTH; i++)
	{
		if (kernel_pd[i] & PAGE_BIT_GLOBAL)
			vpd[i] = kernel_pd[i];
	}

	kp_unlock();

	return pd;
}

/* Free a PD. */
void paging_free_dir(paddr_t pd)
{
	pde_t *pde;
	uint i;

	/* Need to be using kernel pages. */
	kassert(is_using_kernel_page_tables());

	/* Potential deadlock because of our pfree() use. */
	check_palloc_lock();

	if (pd == phys_kernel_pd)
		kpanic("paging_free_dir(): attempted to free kernel page directory");

	/* Free present, non-global page tables. Those have been allocated for this page directory. */
	pde = translate_or_panic(pd);

	for (i = 0; i < PD_LENGTH; i++)
	{
		if ((pde[i] & PAGE_BIT_PRESENT) && (pde[i] & PAGE_BIT_GLOBAL) == 0)
			pfree(pde_get_paddr(pde[i]));
	}

	/* Free the page directory itself. */
	pfree(pd);
}

/* Map physical page p to virtual address v using given flags for page tables in pd. */
void paging_map(paddr_t pd, xvaddr_t v, paddr_t p, pflags_t flags)
{
	int pdi, pti;
	pde_t *pde;
	pte_t *pte;
	paddr_t pt;

	if ((flags & PAGE_BIT_GLOBAL) && pd != phys_kernel_pd)
		kpanic("paging_map(): attempted to use the global flag in non-kernel page directory");

	/* Potential deadlock because of our palloc() use. */
	check_palloc_lock();

	/* We need to read and write some physical pages. This has to be done with kernel page tables. */
	kassert(is_using_kernel_page_tables());

	v = (xvaddr_t)mask_to_page(v);

	pdi = get_pd_index(v);
	pti = get_pt_index(v);

	/* Make sure the PD entry points at our PT. */
	pde = translate_or_panic(pd);
	pde = pde + pdi;

	/* Make sure the we do not overwrite kernel page structures! */
	if (((*pde) & PAGE_BIT_GLOBAL) && pd != phys_kernel_pd)
		kpanic("paging_map(): attempted to overwrite global kernel PD entries in non-kernel PD");

	if (pde_get_paddr(*pde) == PHYS_NULL)
	{
		/* We assume page returned by palloc equals the size of a page table. */
		kassert(PT_LENGTH * sizeof(pte_t) == palloc_get_granularity());

		/* Allocate the page table and clear it. */
		pt = palloc();
		kmemset(ptranslate(pt), 0, PT_LENGTH * sizeof(pte_t));

		/* Write it to the page directory. */
		*pde = pde_construct(pt, 0);
	}

	*pde |= flags | PAGE_BIT_PRESENT;

	/* Modify the PT. */
	pte = translate_or_panic(pde_get_paddr(*pde));
	pte = pte + pti;

	*pte = pte_construct(p, flags | PAGE_BIT_PRESENT);

	/* TODO: Propagate changes to other CPUs here? */
}

/* Get the entry of the virutal address v in page tables pd. */
pte_t paging_get_entry(paddr_t pd, xvaddr_t v)
{
	int pdi, pti;
	pde_t *pde;
	pte_t *pte;

	/* We need to read some physical pages. This has to be done with kernel page tables. */
	kassert(is_using_kernel_page_tables());

	v = (xvaddr_t)mask_to_page(v);

	pdi = get_pd_index(v);
	pti = get_pt_index(v);

	/* Translate the first level. */
	pde = translate_or_panic(pd);
	pde = pde + pdi;

	if (pde_get_paddr(*pde) == PHYS_NULL)
		return PHYS_NULL;

	/* Translate the second level. */
	pte = translate_or_panic(pde_get_paddr(*pde));
	pte = pte + pti;

	/* We can now read the page table. */
	return *pte;
}

/* Get the physical address of the virutal address v in page tables pd. */
paddr_t paging_get(paddr_t pd, xvaddr_t v)
{
	return pte_get_paddr(paging_get_entry(pd, v));
}

static void vmxchg(paddr_t pd, uvaddr_t v, void *buf, size_t num, bool write)
{
	paddr_t p;
	byte *accessible;
	size_t off, batch;
	size_t buf_off = 0;

	/* Need to be using kernel pages. */
	kassert(is_using_kernel_page_tables());

	while (num > 0)
	{
		/* Get the page from the page tables and translate it to an accessible address. */
		p = paging_get(pd, v);
		accessible = translate_or_panic(p);

		/* Calculate how much we have to read/write to this page. */
		off = (size_t)(v - mask_to_page(v));
		batch = kmin(PAGE_SIZE - off, num);

		/* Read/write the bytes. */
		if (write)
			kmemcpy(accessible + off, buf + buf_off, batch);
		else
			kmemcpy(buf + buf_off, accessible + off, batch);

		num -= batch;
		buf_off += batch;

		/* Move onto the next page. */
		v = (uvaddr_t)(mask_to_page(v) + PAGE_SIZE);
	}
}

/* Read num number of bytes into buf from the virtual address v of page tables pd. TODO: Check the address and return some information. */
void vmread(paddr_t pd, uvaddr_t v, void *buf, size_t num)
{
	vmxchg(pd, v, buf, num, false);
}

/* Write num number of bytes from buf into the virtual address v of page tables pd. TODO: Check the address and return some information. */
void vmwrite(paddr_t pd, uvaddr_t v, const void *buf, size_t num)
{
	vmxchg(pd, v, (void*)buf, num, true);
}
