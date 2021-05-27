/* cpu_checkpoint.c - spin checkpoint for CPUs */
#include <kernel/cdefs.h>
#include <kernel/cpu.h>
#include <kernel/cpu_checkpoint.h>
#include <kernel/interrupts.h>
#include <arch/cpu.h>

void cpu_checkpoint_create(struct cpu_checkpoint *cp)
{
	atomic_store(&(cp->num), 0);
}

void cpu_checkpoint_enter(struct cpu_checkpoint *cp)
{
	push_no_interrupts();

	atomic_fetch_add(&(cp->num), 1);

	while(atomic_load(&(cp->num)) != get_nof_active_cpus())
		cpu_relax();

	pop_no_interrupts();
}
