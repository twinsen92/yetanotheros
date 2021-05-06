/* early_paging.c - early paging initialization, with initial paging structures */
/* Only other objects in arch/i386/boot are available in this compilation unit. */

#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <arch/boot_layout.h>
#include <arch/paging.h>

#include "early_debug.h"

static pte_t *page_tables;
static pde_t *new_kernel_pd;
static pte_t *new_kernel_page_tables;
static uint8_t tmp_page[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));

#define is_symbol_aligned_to_page(m) (get_symbol_vaddr(m) == (vaddr_t)mask_to_page(get_symbol_vaddr(m)))
#define calc_paddr(v) (paddr_t)((uint32_t)((vaddr_t)(v) - KERNEL_BASE_VADDR))

static void *enable(void *v)
{
	pte_t *pte = page_tables + (get_pd_index(tmp_page) * PT_LENGTH + get_pt_index(tmp_page));
	*pte = pte_construct(calc_paddr(v), PAGE_BIT_RW | PAGE_BIT_PRESENT);
	asm_invlpg(tmp_page);
	return (void*)(tmp_page + get_sub_page_addr(v));
}

static void map(vaddr_t v, uint32_t flags)
{
	int pdi, pti;
	paddr_t p;
	pde_t *pde;
	pte_t *pte;

	v = (vaddr_t)mask_to_page(v);

	pdi = get_pd_index(v);
	pti = get_pt_index(v);

	/* Make sure the PD entry points at our PT. */
	pde = new_kernel_pd + pdi;
	pde = enable(pde);

	if (pde_get_paddr(*pde) == NULL)
	{
		p = calc_paddr(new_kernel_page_tables + (pdi * PT_LENGTH));
		/* TODO: Is RW needed? */
		*pde = pde_construct(p, PAGE_BIT_GLOBAL | PAGE_BIT_RW | PAGE_BIT_PRESENT);
	}

	/* Modify the PT. */
	pte = new_kernel_page_tables + (pdi * PT_LENGTH + pti);
	pte = enable(pte);

	p = calc_paddr(v);
	*pte = pte_construct(p, flags | PAGE_BIT_GLOBAL | PAGE_BIT_PRESENT);
}

static void map_region(vaddr_t vfrom, vaddr_t vto, uint32_t flags)
{
	while (vfrom < vto)
	{
		map(vfrom, flags);
		vfrom += PAGE_SIZE;
	}
}

static void add_self_ref(int pdi, uint32_t flags)
{
	pde_t *pde;

	pde = new_kernel_pd + pdi;
	pde = enable(pde);

	*pde = pde_construct(calc_paddr(new_kernel_pd), flags | PAGE_BIT_RW | PAGE_BIT_PRESENT);
}

void early_init_kernel_paging(void)
{
	/* We initialize the static variables here because _init() will not have been called yet. */
	page_tables = (pte_t *) 0xffc00000;
	new_kernel_pd = get_symbol_vaddr(__kernel_pd);
	new_kernel_page_tables = get_symbol_vaddr(__kernel_page_tables);

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
	early_kassert(is_symbol_aligned_to_page(__kernel_pd));
	early_kassert(is_symbol_aligned_to_page(__kernel_page_tables));

	/* Create page table entries for all kernel executable sections. */
	map_region(get_symbol_vaddr(__kernel_low_begin), get_symbol_vaddr(__kernel_low_end),
		PAGE_BIT_RW);
	map_region(get_symbol_vaddr(__kernel_mem_ro_begin), get_symbol_vaddr(__kernel_mem_ro_end), 0);
	map_region(get_symbol_vaddr(__kernel_mem_rw_begin), get_symbol_vaddr(__kernel_mem_rw_end),
		PAGE_BIT_RW);
	add_self_ref(CURRENT_PD_INDEX, 0);

	asm_set_cr3(calc_paddr(new_kernel_pd));

	/* Now we can use the whole kernel executable! */
}
