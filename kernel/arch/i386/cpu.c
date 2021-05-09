/* cpu.c - holder of the cpu information */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <arch/cpu.h>

/* We only have one cpu right now. */
static x86_cpu_t cpu;

void init_cpu(void)
{
	cpu.num = 0;
	cpu.magic = X86_CPU_MAGIC;
	cpu_force_cli();
	cpu.int_enabled = false;
	cpu.cli_stack = 0;
}

x86_cpu_t *cpu_current(void)
{
	/* Check if the cpu has been initialized. */
	kassert(cpu.magic == X86_CPU_MAGIC);
	return &cpu;
}

/* kernel/interrupts.h interface */

void push_no_interrupts(void)
{
	x86_cpu_t *cpu = cpu_current();
	uint32_t eflags;

	eflags = cpu_get_eflags();

	cpu_force_cli();

	if (cpu->cli_stack == 0)
		cpu->int_enabled = eflags & EFLAGS_IF;

	cpu->cli_stack += 1;
}

void pop_no_interrupts(void)
{
	x86_cpu_t *cpu = cpu_current();

	if (cpu_get_eflags() & EFLAGS_IF)
		kpanic("pop_no_interrupts(): interrupts enabled");

	cpu->cli_stack -= 1;

	if (cpu->cli_stack < 0)
		kpanic("pop_no_interrupts(): stack underflow");

	if (cpu->cli_stack == 0 && cpu->int_enabled)
		cpu_force_sti();
}
