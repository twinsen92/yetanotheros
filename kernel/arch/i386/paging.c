/* paging.c - memory paging subsystem */

#include <kernel/addr.h>
#include <arch/paging.h>

pde_t *const current_pd = (pde_t *const) 0xfffff000;
pte_t *const current_page_tables = (pte_t *const) 0xffc00000;
pde_t *const kernel_pd = get_symbol_vaddr(__kernel_pd);
pte_t *const kernel_page_tables = get_symbol_vaddr(__kernel_page_tables);
