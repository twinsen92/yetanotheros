/* early_paging.c - early paging initialization, with initial paging structures */
/* Only other objects in arch/i386/boot are available in this compilation unit. */

#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/paging.h>
#include <arch/memlayout.h>
#include <arch/paging.h>

#include "early_debug.h"

static pte_t *page_tables;
static pde_t *new_kernel_pd;
static pte_t *new_kernel_page_tables;
static struct vm_region static_vm_map[VM_NOF_REGIONS];
const struct vm_region *vm_map = static_vm_map;
static uint8_t tmp_page[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));

#define is_symbol_aligned_to_page(m) (get_symbol_vaddr(m) == (vaddr_t)mask_to_page(get_symbol_vaddr(m)))

/* An early page enabler that enables memory in kernel executable through tmp_page. This is done
   because the boot page structures may not map all of the kernel's executable initially. */
static void *early_enable(void *v)
{
	pte_t *pte = page_tables + (get_pd_index(tmp_page) * PT_LENGTH + get_pt_index(tmp_page));
	*pte = pte_construct(km_paddr(v), PAGE_BIT_RW | PAGE_BIT_PRESENT);
	asm_invlpg(tmp_page);
	return (void*)(tmp_page + get_sub_page_addr(v));
}

/* A late page enabler for memory in kernel executable that does nothing because now the kernel
   executable is mapped to virtual memory space entirely. */
static void *late_enable(void *v)
{
	return v;
}

/* Calls appropriate enabler based on early flag. */
static void *enable(void *v, bool early)
{
	if (early)
		return early_enable(v);
	else
		return late_enable(v);
}

/* Maps exactly one page at physical memory 'p' to virutal memory 'v' with given flags. The mapping
   is done on base kernel page structures. */
static void map(vaddr_t v, paddr_t p, uint32_t flags, bool early)
{
	int pdi, pti;
	paddr_t ptp;
	pde_t *pde;
	pte_t *pte;

	v = (vaddr_t)mask_to_page(v);

	pdi = get_pd_index(v);
	pti = get_pt_index(v);

	/* Make sure the PD entry points at our PT. */
	pde = new_kernel_pd + pdi;
	pde = enable(pde, early);

	if (pde_get_paddr(*pde) == PHYS_NULL)
	{
		ptp = km_paddr(new_kernel_page_tables + (pdi * PT_LENGTH));
		*pde = pde_construct(ptp, flags | PAGE_BIT_PRESENT);
	}

	/* Force RW in PDE if one of PTEs has it. */
	if ((flags & PAGE_BIT_RW) && (((*pde) & PAGE_BIT_RW) == 0))
		*pde |= PAGE_BIT_RW;

	/* Modify the PT. */
	pte = new_kernel_page_tables + (pdi * PT_LENGTH + pti);
	pte = enable(pte, early);

	*pte = pte_construct(p, flags | PAGE_BIT_PRESENT);
}

/* Maps a region of physical memory starting from pfrom to pto, at vfrom, with given flags.
   The mapping is done on base kernel page structures. */
static void map_region(vaddr_t vfrom, paddr_t pfrom, paddr_t pto, uint32_t flags, bool early)
{
	while (pfrom < pto)
	{
		map(vfrom, pfrom, flags, early);
		vfrom += PAGE_SIZE;
		pfrom += PAGE_SIZE;
	}
}

/* Maps a region of physical memory based on a struct vm_region object.  The mapping is done on base
   kernel page structures. */
static void map_region_object(const struct vm_region *region, bool early)
{
	map_region(region->vbase, region->pbase, region->pbase + region->size, region->pflags, early);
}

static void mark(vaddr_t v, pflags_t pflags, bool early)
{
	int pdi;
	pde_t *pde;

	v = (vaddr_t)mask_to_page(v);

	pdi = get_pd_index(v);

	/* Make sure the PD entry points at our PT. */
	pde = new_kernel_pd + pdi;
	pde = enable(pde, early);

	*pde |= pflags;
}

static void mark_region(vaddr_t vfrom, vaddr_t vto, pflags_t pflags, bool early)
{
	while (vfrom < vto)
	{
		mark(vfrom, pflags, early);
		vfrom += PAGE_SIZE;
	}
}

static void mark_region_object(const struct vm_region *region, pflags_t pflags, bool early)
{
	mark_region(region->vbase, region->vbase + region->size, region->pflags | pflags, early);
}

/* Adds a self-reference to the kernel page directory at a the kernel page directory entry pdi,
   with given flags. */
static void add_self_ref(int pdi, uint32_t flags, bool early)
{
	pde_t *pde;

	pde = new_kernel_pd + pdi;
	pde = enable(pde, early);

	*pde = pde_construct(km_paddr(new_kernel_pd), flags | PAGE_BIT_RW | PAGE_BIT_PRESENT);
}

/* Boot-time kernel paging structures initialization. */
void early_init_kernel_paging(void)
{
	/* We initialize the static variables here because _init() will not have been called yet. */
	page_tables = (pte_t *) 0xffc00000;
	new_kernel_pd = get_symbol_vaddr(__kernel_pd);
	new_kernel_page_tables = get_symbol_vaddr(__kernel_page_tables);
	create_vm_map(static_vm_map);

	/* Verify that linker symbols are aligned to page size. This is important because we want to
	   separate the entries in page tables. */
	early_kassert(is_symbol_aligned_to_page(__kernel_virtual_offset));
	early_kassert(is_symbol_aligned_to_page(__kernel_low_begin));
	early_kassert(is_symbol_aligned_to_page(__kernel_low_end));
	early_kassert(is_symbol_aligned_to_page(__kernel_mem_ro_begin));
	early_kassert(is_symbol_aligned_to_page(__kernel_mem_ro_end));
	early_kassert(is_symbol_aligned_to_page(__kernel_mem_rw_begin));
	early_kassert(is_symbol_aligned_to_page(__kernel_mem_rw_end));
	early_kassert(is_symbol_aligned_to_page(__kernel_mem_break));
	early_kassert(is_symbol_aligned_to_page(__kernel_ap_entry_begin));
	early_kassert(is_symbol_aligned_to_page(__kernel_ap_entry_end));
	early_kassert(is_symbol_aligned_to_page(__kernel_pd));
	early_kassert(is_symbol_aligned_to_page(__kernel_page_tables));

	/* Verify that our assumptions in memlayout.h about the memory layout are true. */
	early_kassert(KM_VIRT_BASE == get_symbol_vaddr(__kernel_virtual_offset));
	early_kassert(KM_EXEC_VIRT_BASE == get_symbol_vaddr(__kernel_mem_ro_begin));

	/* Create page table entries for the kernel executable and stack sections. */
	for(int i = 0; i < VM_NOF_REGIONS; i++)
	{
		/* We're only mapping static executable data here. */
		if ((vm_map[i].flags & VM_BIT_STATIC) && (vm_map[i].flags & VM_BIT_EXECUTABLE))
			map_region_object(&vm_map[i], true);
	}

	/* Also create a self referencing entry. */
	add_self_ref(CURRENT_PD_INDEX, 0, true);

	/* Use the new page tables. */
	asm_set_cr3(km_paddr(new_kernel_pd));

	/* Now that we can access all of the kernel executable, time to map other regions. */
	for(int i = 0; i < VM_NOF_REGIONS; i++)
	{
		/* We're only mapping static non-executable data here. */
		if ((vm_map[i].flags & VM_BIT_STATIC) && !(vm_map[i].flags & VM_BIT_EXECUTABLE))
			map_region_object(&vm_map[i], false);
	}

	/* Fill in the page table addresses in the page directory. */
	for (unsigned int i = 0; i < PD_LENGTH; i++)
	{
		if (pde_get_paddr(new_kernel_pd[i]) == PHYS_NULL)
			new_kernel_pd[i] |= km_paddr(new_kernel_page_tables + (i * PT_LENGTH));
	}

	/* Apply the region the paging flags to all underlying directory entries. We also mark them as
	   present, because each entry contains a valid physical address to a page table.

	   This will basically make the kernel page directory constant for the whole time the system
	   is up, which allows us to copy the global (kernel) entries ad-hoc to newly created user
	   process' page directories.*/
	for(int i = 0; i < VM_NOF_REGIONS; i++)
	{
		mark_region_object(&vm_map[i], PAGE_BIT_PRESENT, false);
	}

	/* Doesn't hurt to flush TLB... */
	asm_set_cr3(km_paddr(new_kernel_pd));
}
