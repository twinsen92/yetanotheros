/* arch/i386/paging/ipi.c - paging IPI communication subsystem */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/cpu.h>
#include <arch/cpu.h>
#include <arch/interrupts.h>
#include <arch/memlayout.h>
#include <arch/paging.h>
#include <arch/cpu/apic.h>

/* IPI spinlock. While held, only the CPU holding the lock can issue INT_FLUSH_TLB_IPIs. */
static struct cpu_spinlock ipi_spinlock;
static paddr_t ipi_updated_pd;
static vaddr_t ipi_updated_page;
static bool ipi_global;
static atomic_bool ipi_received;

static void ipi_flush_tlb_handler(__unused struct isr_frame *frame)
{
	uint32_t cr3;
	paddr_t phys_kernel_pd;

	phys_kernel_pd = vm_map_rev_walk(kernel_pd, true);

	preempt_disable();

	cr3 = cpu_get_cr3();

	if (ipi_global && ipi_updated_pd == phys_kernel_pd && cr3 != phys_kernel_pd)
	{
		/* If kernel pages were modified with a global bit somehow, we need to switch to them and
		   then go back to the page directory we were using before. That will propagate global
		   pages. */
		cpu_set_cr3(phys_kernel_pd);
		cpu_set_cr3(cr3);
	}
	else if (ipi_updated_pd == cr3)
	{
		/* Check if we can just update one page. Otherwise, flush TLB entirely. */
		if (ipi_updated_page)
			asm_invlpg(ipi_updated_page);
		else
			cpu_flush_tlb();
	}

	preempt_enable();

	/* Send an EOI and atomically set the received flag. */
	lapic_eoi();
	atomic_store(&ipi_received, true);
}

static void unsafe_propagate_to_cpu(struct x86_cpu *cpu)
{
	/* Ignore inactive CPUs. */
	if (atomic_load(&(cpu->active)) == false)
		return;

	/* Issue the IPI and actively wait for the other CPU to set the received flag. */
	atomic_store(&ipi_received, false);
	lapic_ipi(cpu->lapic_id, INT_FLUSH_TLB_IPI, 0);
	lapic_ipi_wait();
	while (atomic_load(&ipi_received) == false)
		cpu_relax();
}

/* Initializes the paging IPI subsystem. */
void init_paging_ipi(void)
{
	cpu_spinlock_create(&ipi_spinlock, "flush TLB IPI");
	isr_set_handler(INT_FLUSH_TLB_IPI, ipi_flush_tlb_handler);
}

/* Propagates changes in page tables to other CPUs.
   pd - the parent page directory's physical address,
   v - non-NULL if we only modified this particular page,
   global - true if the changes contain the global bit. */
void paging_propagate_changes(paddr_t pd, vaddr_t v, bool global)
{
	cpu_spinlock_acquire(&ipi_spinlock);

	ipi_updated_pd = pd;
	ipi_updated_page = v;
	ipi_global = global;
	cpu_enumerate_other_cpus(unsafe_propagate_to_cpu);

	cpu_spinlock_release(&ipi_spinlock);
}
