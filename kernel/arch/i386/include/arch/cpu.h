/* arch/cpu.h - x86 cpu state information */
#ifndef ARCH_I386_CPU_H
#define ARCH_I386_CPU_H

#include <kernel/cdefs.h>
#include <kernel/thread.h>
#include <arch/interrupts.h>
#include <arch/proc.h>
#include <arch/thread.h>
#include <arch/cpu/apic.h>
#include <arch/cpu/apic_types.h>
#include <arch/cpu/gdt.h>
#include <arch/cpu/idt.h>
#include <arch/cpu/seg_types.h>
#include <arch/cpu/selectors.h>

#define X86_CPU_MAGIC 0x86

struct x86_cpu
{
	int magic;
	int num;
	atomic_bool active;
	lapic_id_t lapic_id;

	vaddr_t stack_top;
	size_t stack_size;

	/* Interrupts */

	bool int_enabled; /* Interrupts state when cli_stack was 0. */
	int cli_stack; /* Number of cli push operations. */

	/* Segmentation */

	struct dtr gdtr;
	seg_t gdt[YAOS2_GDT_NOF_ENTRIES];
	volatile struct tss tss;

	/* Scheduler fields. */
	int preempt_disabled;
	struct thread scheduler_thread;
	struct arch_thread scheduler_arch_thread;
	struct thread *scheduler;
	struct thread *thread;
};

extern lapic_id_t boot_lapic_id;

/* Adds a CPU. */
void cpu_add(lapic_id_t lapic_id);

/* Sets current CPU as the boot CPU. This sets boot_lapic_id. */
void cpu_set_boot_cpu(void);

/* Enumerates other CPUs. Call with interrupts disabled. */
void cpu_enumerate_other_cpus(void (*receiver)(struct x86_cpu *));

/* Gets the current CPU object. */
struct x86_cpu *cpu_current(void);

/* Checks if current CPU is boot CPU. */
#define is_boot_cpu() (boot_lapic_id == lapic_get_id())

/* Set current CPU's active flag. */
void cpu_set_active(bool flag);

/* Flush TLB on current CPU. */
void cpu_flush_tlb(void);

/* Set CR3 on current CPU. Note that this function avoids unnecessary CR3 switches. Use
   cpu_flush_tlb() to perform flushes. Returns previous CR3 value. */
paddr_t cpu_set_cr3(paddr_t cr3);

static inline paddr_t cpu_get_cr3(void)
{
	paddr_t cr3;
	asm volatile ("movl %%cr3, %0" : "=r" (cr3));
	return cr3;
}

/* Setup TSS for execution of the given thread on the CPU. */
void cpu_setup_tss(struct x86_cpu *cpu, struct thread *thread);

/* A cli that does not update CPU object */
#define cpu_force_cli() asm volatile("cli" : : : "memory")

/* An sti that does not update CPU object */
#define cpu_force_sti() asm volatile("sti" : : : "memory")

/* Sets the desired interrupts state for current CPU. Returns previous state. Assumes the current
   CPU is the given CPU. */
static inline bool cpu_set_interrupts_with_cpu(struct x86_cpu *cpu, bool state)
{
	bool prev = cpu->int_enabled;

	if (state)
	{
		cpu->int_enabled = true;
		cpu->cli_stack = 0;
		cpu_force_sti();
	}
	else
	{
		cpu_force_cli();
		cpu->int_enabled = false;
		cpu->cli_stack = 0;
	}

	return prev;
}

/* Sets the desired interrupts state for current CPU. Returns previous state. */
#define cpu_set_interrupts(state) cpu_set_interrupts_with_cpu(cpu_current(), (state))

/* Returns the current state of EFLAGS. */
static inline uint32_t cpu_get_eflags()
{
	uint32_t val;

	asm volatile (
		"pushfl\n"
		"popl %%eax\n"
		"movl %%eax, %0\n"
		:"=m" (val)
		);

	return val;
}

#define EFLAGS_IF 0x200

/* Checks whether the current CPU has the CPUID instruction. */
static inline bool cpu_has_cpuid(void)
{
	uint32_t eax;

	asm volatile (
		"pushfl\n"
		"pushfl\n"
		"xorl $0x00200000, (%%esp)\n"
		"popfl\n"
		"pushfl\n"
		"popl %%eax\n"
		"xorl (%%esp), %%eax\n"
		"popfl\n"
		"andl $0x00200000, %%eax\n"
		"movl %%eax, %0\n"
		: "=r" (eax)
		:
		: "eax"
	);

	return eax > 0;
}

/* Issues the CPUID instruction. */
static inline void cpuid (uint32_t cat, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
	asm volatile (
		"cpuid"
		: "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
		: "0" (cat)
	);
}

#define CPUID_FEATURES 1

#endif
