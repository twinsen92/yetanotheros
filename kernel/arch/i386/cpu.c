/* cpu.c - holder of the cpu information */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/init.h>
#include <kernel/interrupts.h>
#include <kernel/utils.h>
#include <arch/apic.h>
#include <arch/cpu.h>
#include <arch/memlayout.h>
#include <arch/paging.h>

#define MAX_CPUS 8
#define BOOT_CPU 0

/* We only have one cpu right now. */
lapic_id_t boot_lapic_id;
static struct x86_cpu cpus[MAX_CPUS];
static unsigned int nof_cpus = 0;
static atomic_uint nof_active_cpus = 0;

#define verify_cpu(cpu) ((cpu)->magic == X86_CPU_MAGIC)

/* Adds a CPU. */
void cpu_add(lapic_id_t lapic_id)
{
	if (nof_cpus == MAX_CPUS)
		kpanic("cpu_add(): too many CPUs");

	/* We're pretty relaxed in this function for a reason... */
	kassert(is_yaos2_initialized() == false);

	cpus[nof_cpus].magic = X86_CPU_MAGIC;
	cpus[nof_cpus].num = nof_cpus;
	cpus[nof_cpus].active = false;
	cpus[nof_cpus].lapic_id = lapic_id;

	cpus[nof_cpus].stack_top = get_symbol_vaddr(__boot_stack_top);
	cpus[nof_cpus].stack_size = get_symbol_vaddr(__boot_stack_top) - get_symbol_vaddr(__boot_stack_bottom);

	cpus[nof_cpus].int_enabled = false;
	cpus[nof_cpus].cli_stack = 0;

	if (cpu_has_cpuid() == false)
		kpanic("init_cpu(): CPU does not support CPUID");

	kmemset(&(cpus[nof_cpus].scheduler_thread), 0, sizeof(cpus[nof_cpus].scheduler_thread));
	cpus[nof_cpus].scheduler = NULL;
	cpus[nof_cpus].thread = NULL;

	nof_cpus++;
}

/* Sets current CPU as the boot CPU. This sets boot_lapic_id. */
void cpu_set_boot_cpu(void)
{
	/* We do not want to do this after initialization... */
	kassert(is_yaos2_initialized() == false);
	boot_lapic_id = lapic_get_id();
}

/* Get the number of CPUs. */
unsigned int get_nof_cpus(void)
{
	return nof_cpus;
}

/* Get the number of active CPUs. */
unsigned int get_nof_active_cpus(void)
{
	return atomic_load(&nof_active_cpus);
}

/* Enumerates other CPUs. Call with interrupts disabled. */
void cpu_enumerate_other_cpus(void (*receiver)(struct x86_cpu *))
{
	lapic_id_t cur_lapic_id;

	/* This is dangerous with interrupts on. */
	if (cpu_get_eflags() & EFLAGS_IF)
		kpanic("cpu_current_or_null(): called with interrupts enabled");

	cur_lapic_id = lapic_get_id();

	for (int i = 0; i < nof_cpus; i++)
		if (verify_cpu(&cpus[i]) && cpus[i].lapic_id != cur_lapic_id)
			receiver(&cpus[i]);
}

/* Gets the current CPU object or NULL if un-initialized. */
struct x86_cpu *cpu_current_or_null(void)
{
	lapic_id_t lapic_id;

	/* We can do this check earlier because if there are no CPUs registered, we don't actually
	   have to be worried of getting rescheduled... */
	if (nof_cpus == 0)
		return NULL;

	/* Need to be called with interrupts off so that we don't get rescheduled between checking
	   the LAPIC ID and scanning the cpus table. */
	if (cpu_get_eflags() & EFLAGS_IF)
		kpanic("cpu_current_or_null(): called with interrupts enabled");

	lapic_id = lapic_get_id();

	for (int i = 0; i < nof_cpus; i++)
		if (verify_cpu(&cpus[i]) && cpus[i].lapic_id == lapic_id)
			return &cpus[i];

	return NULL;
}

/* Gets the current CPU object. */
struct x86_cpu *cpu_current(void)
{
	struct x86_cpu *cpu;

	if (cpu_get_eflags() & EFLAGS_IF)
		kpanic("cpu_current(): called with interrupts enabled");

	cpu = cpu_current_or_null();

	if (cpu)
		return cpu;

	kpanic("cpu_current(): called from an unregistered CPU");
}

/* Set current CPU's active flag. */
void cpu_set_active(bool flag)
{
	struct x86_cpu *cpu;

	push_no_interrupts();
	cpu = cpu_current();
	if (atomic_load(&(cpu->active)) == false)
		atomic_fetch_add(&nof_active_cpus, 1);
	atomic_store(&(cpu->active), flag);
	pop_no_interrupts();

}

/* Flush TLB on current CPU. */
void cpu_flush_tlb(void)
{
	paddr_t cr3;

	/* Need to turn off interrupts so that we're not rescheduled during this process. */
	push_no_interrupts();

	cr3 = cpu_get_cr3();

	asm volatile ("movl %0, %%cr3" : : "r" (cr3) : "memory");

	pop_no_interrupts();
}

/* Set CR3 on current CPU. */
paddr_t cpu_set_cr3(paddr_t cr3)
{
	paddr_t prev_cr3;

	/* Need to turn off interrupts so that we're not rescheduled during this process. */
	push_no_interrupts();

	prev_cr3 = cpu_get_cr3();

	/* Check if we're switching to the same CR3. */
	if (cr3 == prev_cr3)
		goto _cpu_set_cr3_redundant;

	asm volatile ("movl %0, %%cr3" : : "r" (cr3) : "memory");

_cpu_set_cr3_redundant:
	pop_no_interrupts();

	return prev_cr3;
}

/* kernel/interrupts.h interface */

void push_no_interrupts(void)
{
	struct x86_cpu *cpu;
	uint32_t eflags;

	/* If we have been called before CPUs have been enumerated, we cannot call cpu_current(). In
	   that case, we assume the interrupts are off. */
	if (nof_cpus == 0)
	{
		kassert((cpu_get_eflags() & EFLAGS_IF) == 0);
		return;
	}

	eflags = cpu_get_eflags();

	cpu_force_cli();
	cpu = cpu_current();

	if (cpu->cli_stack == 0)
		cpu->int_enabled = eflags & EFLAGS_IF;

	cpu->cli_stack += 1;
}

void pop_no_interrupts(void)
{
	struct x86_cpu *cpu;

	/* If we have been called before CPUs have been enumerated, we cannot call cpu_current(). In
	   that case, we assume the interrupts are off. */
	if (nof_cpus == 0)
	{
		kassert((cpu_get_eflags() & EFLAGS_IF) == 0);
		return;
	}

	/* Calling pop_no_interrupts before push_no_interrupts is an error! */
	if (cpu_get_eflags() & EFLAGS_IF)
		kpanic("pop_no_interrupts(): interrupts enabled");

	cpu = cpu_current();
	cpu->cli_stack -= 1;

	if (cpu->cli_stack < 0)
		kpanic("pop_no_interrupts(): stack underflow");

	if (cpu->cli_stack == 0 && cpu->int_enabled)
		cpu_force_sti();
}
