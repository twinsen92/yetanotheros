/* cpu.c - holder of the cpu information */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/init.h>
#include <arch/apic.h>
#include <arch/cpu.h>

#define MAX_CPUS 8
#define BOOT_CPU 0

/* We only have one cpu right now. */
static x86_cpu_t cpus[MAX_CPUS];
static int nof_cpus = 0;

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
	/* TODO: We assume that the CPU starts with interrupts disabled here. Is that correct? */
	cpus[nof_cpus].int_enabled = false;
	cpus[nof_cpus].cli_stack = 0;

	if (cpu_has_cpuid() == false)
		kpanic("init_cpu(): CPU does not support CPUID");

	nof_cpus++;
}

/* Get the number of CPUs. */
int get_nof_cpus(void)
{
	return nof_cpus;
}

/* Gets the current CPU object or NULL if un-initialized. */
x86_cpu_t *cpu_current_or_null(void)
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
		if (cpus[i].lapic_id == lapic_id)
			return &cpus[i];

	return NULL;
}

/* Gets the current CPU object. */
x86_cpu_t *cpu_current(void)
{
	x86_cpu_t *cpu;

	if (cpu_get_eflags() & EFLAGS_IF)
		kpanic("cpu_current(): called with interrupts enabled");

	cpu = cpu_current_or_null();

	if (cpu)
		return cpu;

	kpanic("cpu_current(): called from an unregistered CPU");
}

/* kernel/interrupts.h interface */

void push_no_interrupts(void)
{
	x86_cpu_t *cpu;
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
	x86_cpu_t *cpu;

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
