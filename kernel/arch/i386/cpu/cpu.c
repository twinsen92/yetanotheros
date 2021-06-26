/* cpu/cpu.c - holder of the cpu information */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/cpu.h>
#include <kernel/debug.h>
#include <kernel/init.h>
#include <kernel/utils.h>
#include <arch/cpu.h>
#include <arch/memlayout.h>
#include <arch/paging.h>
#include <arch/cpu/apic.h>

#define MAX_CPUS 8
#define BOOT_CPU 0

static struct x86_cpu cpus[MAX_CPUS];

/* Initial CPU, when we have not enumerated CPUs yet. */
static struct x86_cpu initial_cpu = {
	.magic = X86_CPU_MAGIC,
	.num = 0,
	.active = false,
	.lapic_id = 0xff,

	.int_enabled = false,
	.cli_stack = 0,

	.preempt_disabled = 0,
	.scheduler = NULL,
	.thread = NULL,
};

lapic_id_t boot_lapic_id;

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

	if (initial_cpu.int_enabled || initial_cpu.cli_stack > 0 || initial_cpu.preempt_disabled > 0)
		kpanic("cpu_add(): initial CPU has state information");

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

	cpus[nof_cpus].preempt_disabled = 0;
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

/* Enumerates other CPUs. Call with interrupts disabled. */
void cpu_enumerate_other_cpus(void (*receiver)(struct x86_cpu *))
{
	lapic_id_t cur_lapic_id;

	/* Check for calls with preemption enabled. */
	preempt_disable();

	cur_lapic_id = lapic_get_id();

	for (unsigned int i = 0; i < nof_cpus; i++)
		if (verify_cpu(&cpus[i]) && cpus[i].lapic_id != cur_lapic_id)
			receiver(&cpus[i]);

	preempt_enable();
}

/* Gets the current CPU object. */
struct x86_cpu *cpu_current(void)
{
	lapic_id_t lapic_id;

	/* We can do this check earlier because if there are no CPUs registered, we don't actually
	   have to be worried of getting rescheduled... */
	if (nof_cpus == 0)
		return &initial_cpu;

	lapic_id = lapic_get_id();

	for (unsigned int i = 0; i < nof_cpus; i++)
		if (verify_cpu(&cpus[i]) && cpus[i].lapic_id == lapic_id)
			return &cpus[i];

	kpanic("cpu_current(): called from an unregistered CPU");
}

/* Set current CPU's active flag. */
void cpu_set_active(bool flag)
{
	struct x86_cpu *cpu;

	preempt_disable();
	cpu = cpu_current();
	if (atomic_load(&(cpu->active)) == false)
		atomic_fetch_add(&nof_active_cpus, 1);
	atomic_store(&(cpu->active), flag);
	preempt_enable();
}

/* Flush TLB on current CPU. */
void cpu_flush_tlb(void)
{
	paddr_t cr3;

	/* Need to turn off interrupts so that we're not rescheduled during this process. */
	preempt_disable();

	cr3 = cpu_get_cr3();

	asm volatile ("movl %0, %%cr3" : : "r" (cr3) : "memory");

	preempt_enable();
}

/* Set CR3 on current CPU. */
paddr_t cpu_set_cr3(paddr_t cr3)
{
	paddr_t prev_cr3;

	/* Need to turn off interrupts so that we're not rescheduled during this process. */
	preempt_disable();

	prev_cr3 = cpu_get_cr3();

	/* Check if we're switching to the same CR3. */
	if (cr3 == prev_cr3)
		goto _cpu_set_cr3_redundant;

	asm volatile ("movl %0, %%cr3" : : "r" (cr3) : "memory");

_cpu_set_cr3_redundant:
	preempt_enable();

	return prev_cr3;
}

/* kernel/cpu.h interface */

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

void push_no_interrupts(void)
{
	struct x86_cpu *cpu;
	uint32_t eflags = cpu_get_eflags();

	/* If we have been called before CPUs have been enumerated, we cannot call cpu_current(). In
	   that case, we assume the interrupts are off. */
	if (nof_cpus == 0)
	{
		kassert((eflags & EFLAGS_IF) == 0);
		return;
	}

	cpu_force_cli();
	cpu = cpu_current();

	if (cpu->cli_stack == 0)
		cpu->int_enabled = eflags & EFLAGS_IF;

	cpu->cli_stack += 1;
}

void pop_no_interrupts(void)
{
	struct x86_cpu *cpu;
	uint32_t eflags = cpu_get_eflags();

	/* If we have been called before CPUs have been enumerated, we cannot call cpu_current(). In
	   that case, we assume the interrupts are off. */
	if (nof_cpus == 0)
	{
		kassert((eflags & EFLAGS_IF) == 0);
		return;
	}

	/* Calling pop_no_interrupts before push_no_interrupts is an error! */
	if (eflags & EFLAGS_IF)
		kpanic("pop_no_interrupts(): interrupts enabled");

	cpu = cpu_current();
	cpu->cli_stack -= 1;

	if (cpu->cli_stack < 0)
		kpanic("pop_no_interrupts(): stack underflow");

	if (cpu->cli_stack == 0 && cpu->int_enabled)
		cpu_force_sti();
}

/* Disable preemption on the CPU. Can be called multiple times. */
void preempt_disable(void)
{
	struct x86_cpu *cpu;

	/* If we have been called before CPUs have been enumerated, we cannot call cpu_current(). In
	   that case, we assume the interrupts are off. */
	if (nof_cpus == 0)
	{
		kassert((cpu_get_eflags() & EFLAGS_IF) == 0);
		return;
	}

	/* Need to turn off interrupts so that we're not rescheduled during this process. */
	push_no_interrupts();
	cpu = cpu_current();
	cpu->preempt_disabled++;
	pop_no_interrupts();
}

/* Enable preemption on the CPU. Has to be called as many times as preempt_disable() to actually
   enable preemption. */
void preempt_enable(void)
{
	struct x86_cpu *cpu;

	/* If we have been called before CPUs have been enumerated, we cannot call cpu_current(). In
	   that case, we assume the interrupts are off. */
	if (nof_cpus == 0)
	{
		kassert((cpu_get_eflags() & EFLAGS_IF) == 0);
		return;
	}

	/* Need to turn off interrupts so that we're not rescheduled during this process. */
	push_no_interrupts();
	cpu = cpu_current();
	cpu->preempt_disabled--;

	if (cpu->preempt_disabled < 0)
		kpanic("preempt_enable(): underflow");

	pop_no_interrupts();
}
