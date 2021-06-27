/* arch/i386/paging/kernel.c - kernel page tables management */
#include <kernel/addr.h>
#include <kernel/cpu.h>
#include <arch/cpu.h>
#include <arch/memlayout.h>
#include <arch/paging.h>
#include <arch/paging_types.h>

pde_t *const current_pd = (pde_t *const) 0xfffff000;
pte_t *const current_page_tables = (pte_t *const) 0xffc00000;
pde_t *const kernel_pd = get_symbol_vaddr(__kernel_pd);
pte_t *const kernel_page_tables = get_symbol_vaddr(__kernel_page_tables);

/* Kernel page tables lock. Acquire this to modify kernel page tables. */
static struct cpu_spinlock kp_spinlock;

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

/* Map one physical page to one virtual page in kernel page tables. */
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

	paging_map(p_kernel_pd, v, p, flags);
	/* Notify other CPUs of these changes. */
	paging_propagate_changes(p_kernel_pd, v, flags & PAGE_BIT_GLOBAL);
	/* If we think we're not going to switch CR3, we should do INVLPG */
	if (prev_cr3 == p_kernel_pd)
		asm_invlpg(v);
	cpu_set_cr3(prev_cr3);

	cpu_spinlock_release(&kp_spinlock);
}
