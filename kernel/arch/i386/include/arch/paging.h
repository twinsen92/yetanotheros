/* cpu/paging.h - arch dependent declarations of x86 paging subsystem */
#ifndef ARCH_I386_PAGING_H
#define ARCH_I386_PAGING_H

#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/paging.h>
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

#define PD_LENGTH (PAGE_SIZE / sizeof(pde_t))
#define PT_LENGTH (PAGE_SIZE / sizeof(pde_t))

/* Utility macros. */

#define get_pd_index(p) (((uint32_t)(p)) >> 22)
#define get_pt_index(p) (((uint32_t)(p)) >> 12 & 0x03FF)
#define get_pd_entry(p) (current_pd + get_pd_index(p))
#define get_kernel_pd_entry(p) (kernel_pd + get_pd_index(p))
#define get_pt_entry(p) (current_page_tables + (get_pd_index(p) * PT_LENGTH + get_pt_index(p)))
#define get_kernel_pt_entry(p) (kernel_page_tables + (get_pd_index(p) * PT_LENGTH + get_pt_index(p)))

#define mask_to_page_table(x) ((uintptr_t)(x) & (uintptr_t)0xffc00000)
#define is_aligned_to_page_table(x) (((uintptr_t)(x)) == mask_to_page_table(x))

#define asm_set_cr3(v) asm volatile ("movl %0, %%cr3" : : "r" ((v)) : "memory")
#define asm_invlpg(v) asm volatile ("invlpg (%0)" : : "r" ((v)) : "memory")
#define asm_flush_tlb() asm volatile ("movl %%cr3, %eax; movl %eax, %%cr3" : : : "eax", "memory")

/* Returns true if CR3 currently contains the kernel page directory. */
static inline bool is_using_kernel_page_tables(void)
{
	paddr_t pd;
	asm volatile ("movl %%cr3, %0" : "=r" (pd));
	return pd == km_paddr(kernel_pd);
}

/*
	General paging utilities.
*/

/* Allocate a new PD. */
paddr_t paging_alloc_dir(void);

/* Free a PD. */
void paging_free_dir(paddr_t pd);

/* Map physical page p to virtual address v using given flags for page tables in pd. */
void paging_map(paddr_t pd, vaddr_t v, paddr_t p, pflags_t flags);

/* Get the physical address of the virutal address v in page tables pd. */
paddr_t paging_get(paddr_t pd, vaddr_t v);

/* Write num number of bytes from buf into the virtual address v of page tables pd. */
void vmwrite(paddr_t pd, vaddr_t v, const void *buf, size_t num);

/*
	Kernel page tables management.
*/

/* Initializes the interface of paging.h */
void init_paging(void);

/* Check if we're holding the kernel page table's lock. This is used to avoid dead-locks. */
bool kp_lock_held(void);

/* Map one physical page to one virtual page in kernel page tables. */
void kp_map(vaddr_t v, paddr_t p);

/*
	Paging inter-processor communication.
*/

/* Initializes the paging IPI subsystem. */
void init_paging_ipi(void);

/* Propagates changes in page tables to other CPUs.
   pd - the parent page directory's physical address,
   v - non-NULL if we only modified this particular page,
   global - true if the changes contain the global bit. */
void paging_propagate_changes(paddr_t pd, vaddr_t v, bool global);

#endif
