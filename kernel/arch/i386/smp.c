/* smp.c - SMP related stuff - AP entry, IPIs */

#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/heap.h>
#include <kernel/utils.h>
#include <arch/apic.h>
#include <arch/cpu.h>
#include <arch/init.h>
#include <arch/interrupts.h>
#include <arch/memlayout.h>
#include <arch/paging.h>

packed_struct ap_entry_args
{
	vaddr32_t stack_top;
	vaddr32_t stack_bottom;
	paddr32_t page_directory;
	vaddr32_t entry;
};

extern symbol_t ap_start; /* ap_entry.S */
extern symbol_t ap_entry_args; /* ap_entry.S */

/* Relocated pointers. */

static paddr_t relocated_entry;
static struct ap_entry_args *relocated_args;

static void ipi_panic_handler(__unused struct isr_frame *frame)
{
	cpu_force_cli();

	while (1)
		cpu_relax();

	__builtin_unreachable();
}

static void setup_ap(struct x86_cpu *ap)
{
	ap->stack_top = kalloc(HEAP_NORMAL, 16, 4096);
	ap->stack_size = 4096;
}

/* Initialize SMP stuff. */
void init_smp(void)
{
	void *original;
	void *relocated;
	size_t size;

	original = KM_VIRT_AP_ENTRY_BASE;
	relocated = km_ap_entry_reloc(KM_VIRT_AP_ENTRY_BASE);
	size = (size_t)(KM_VIRT_AP_ENTRY_END - KM_VIRT_AP_ENTRY_BASE);

	kmemmove(relocated, original, size);

	relocated_entry = km_paddr(km_ap_entry_reloc(get_symbol_vaddr(ap_start)));
	relocated_args = km_ap_entry_reloc(get_symbol_vaddr(ap_entry_args));

	/* Allocate stacks for APs. */
	cpu_enumerate_other_cpus(setup_ap);

	isr_set_handler(INT_PANIC_IPI, ipi_panic_handler);
}

static void start_ap(struct x86_cpu *ap)
{
	/* Start APs one by one. */

	/* Make sure to fix this place for long mode... */
	kassert(sizeof(vaddr32_t) == sizeof(vaddr_t));

	/* Fill out the args. */
	relocated_args->stack_top = (vaddr32_t) ap->stack_top;
	relocated_args->stack_bottom = (vaddr32_t) ap->stack_top + 4096;
	relocated_args->page_directory = (paddr32_t) vm_map_rev_walk(kernel_pd, true);
	relocated_args->entry = (vaddr32_t) cpu_entry;

	/* Send a startup IPI. */
	lapic_start_ap(ap->lapic_id, (uint16_t)(relocated_entry & 0xffff));

	/* Wait for the CPU to mark itself active. */
	while (atomic_load(&(ap->active)) == false);
}

/* Enumerate APs and start them one by one. */
void start_aps(void)
{
	cpu_enumerate_other_cpus(start_ap);
}
