/* paging.c - memory paging subsystem */

#include <kernel/cdefs.h>
#include <arch/memlayout.h>
#include <arch/paging.h>

pde_t *const current_pd = (pde_t *const) 0xfffff000;
pte_t *const current_page_tables = (pte_t *const) 0xffc00000;
pde_t *const kernel_pd = get_symbol_vaddr(__kernel_pd);
pte_t *const kernel_page_tables = get_symbol_vaddr(__kernel_page_tables);

bool is_using_kernel_page_tables(void)
{
	paddr_t pd;
	asm volatile ("movl %%cr3, %0" : "=r" (pd));
	return pd == km_paddr(kernel_pd);
}
