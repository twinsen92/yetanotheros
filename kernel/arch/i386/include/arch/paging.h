/* paging.h - arch dependent declarations of x86 paging subsystem */
#ifndef ARCH_I386_PAGING_H
#define ARCH_I386_PAGING_H

#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <arch/memlayout.h>
#include <arch/paging_types.h>

/* We want to keep the paging structures of the currently used page directory and the base kernel
   paging structures always mapped to the virtual memory space. We will use the two last directory
   entries for that purpose. We will keep the current page directory mapped at 0xfffff000 and
   the first page table at 0xffc00000. This will waste 4 MiB of the virtual memory space in every
   process. TODO: Keep a map of this region somewhere. */
#define CURRENT_PD_INDEX	(PD_LENGTH - 1)

extern pde_t *const current_pd;
extern pte_t *const current_page_tables;
extern pde_t *const kernel_pd;
extern pte_t *const kernel_page_tables;

/* Dimensions of paging structures. */

#define PAGE_SIZE 4096
#define PD_LENGTH (PAGE_SIZE / sizeof(pde_t))
#define PT_LENGTH (PAGE_SIZE / sizeof(pde_t))

/* Utility macros. */

#define get_pd_index(p) (((uint32_t)(p)) >> 22)
#define get_pt_index(p) (((uint32_t)(p)) >> 12 & 0x03FF)
#define get_pd_entry(p) (current_pd + get_pd_index(p))
#define get_kernel_pd_entry(p) (kernel_pd + get_pd_index(p))
#define get_pt_entry(p) (current_page_tables + (get_pd_index(p) * PT_LENGTH + get_pt_index(p)))
#define get_kernel_pt_entry(p) (kernel_page_tables + (get_pd_index(p) * PT_LENGTH + get_pt_index(p)))
#define get_sub_page_addr(p) (((uint32_t)(p)) & 0x00000fff)

#define align_to_next_page(x) ((uint32_t)(x + PAGE_SIZE - 1) & ((uint32_t)0xfffff000))
#define mask_to_page(x) ((uint32_t)(x) & (uint32_t)0xfffff000)
#define mask_to_page_table(x) ((uint32_t)(x) & (uint32_t)0xffc00000)

#define is_aligned_to_page_size(x) (((uint32_t)(x)) == mask_to_page(x))
#define is_aligned_to_page_table(x) (((uint32_t)(x)) == mask_to_page_table(x))

#define asm_set_cr3(v) asm volatile ("movl %0, %%cr3" : : "r" ((v)) : "memory")
#define asm_invlpg(v) asm volatile ("invlpg (%0)" : : "r" ((v)) : "memory")

/* Returns true if CR3 currently contains the kernel page directory. */
bool is_using_kernel_page_tables(void);

#endif
