/* cpu.h - x86 cpu state information */
#ifndef ARCH_I386_CPU_H
#define ARCH_I386_CPU_H

#include <kernel/cdefs.h>
#include <arch/apic_types.h>
#include <arch/gdt.h>
#include <arch/idt.h>
#include <arch/interrupts.h>
#include <arch/proc.h>
#include <arch/seg_types.h>
#include <arch/selectors.h>
#include <arch/thread.h>

#define X86_CPU_MAGIC 0x86

typedef struct
{
	int magic;
	int num;
	atomic_bool active;
	lapic_id_t lapic_id;

	/* Interrupts */

	bool int_enabled; /* Interrupts state when cli_stack was 0. */
	int cli_stack; /* Number of cli push operations. */

	/* Segmentation */

	dtr_t gdtr;
	seg_t gdt[YAOS2_GDT_NOF_ENTRIES];
	volatile tss_t tss;

	/* This flag means the kernel's page tables changed and have to be reloaded. */
	atomic_bool kvm_changed;

	/* Scheduler fields. */
	struct x86_thread *scheduler;
	struct x86_thread *thread;
}
x86_cpu_t;

/* Adds a CPU. */
void cpu_add(lapic_id_t lapic_id);

/* Get the number of CPUs. */
int get_nof_cpus(void);

/* Gets the current CPU object or NULL if un-initialized. */
x86_cpu_t *cpu_current_or_null(void);

/* Gets the current CPU object. */
x86_cpu_t *cpu_current(void);

/* Set current CPU's active flag. */
void cpu_set_active(bool flag);

/* Broadcast the kvm_changed flag. */
void cpu_kvm_changed(void);

/* Flush TLB on current CPU. */
void cpu_flush_tlb(void);

/* Set CR3 on assumed, current CPU. Note that this function avoids unnecessary CR3 switches. Use
   cpu_flush_tlb() to perform flushes. Returns previous CR3 value. */
paddr_t cpu_set_cr3_with_cpu(x86_cpu_t *cpu, paddr_t cr3);

/* Set CR3 on current CPU. Note that this function avoids unnecessary CR3 switches. Use
   cpu_flush_tlb() to perform flushes. Returns previous CR3 value. */
paddr_t cpu_set_cr3(paddr_t cr3);

static inline paddr_t cpu_get_cr3(void)
{
	paddr_t cr3;
	asm volatile ("movl %%cr3, %0" : "=r" (cr3));
	return cr3;
}

/* A cli that does not update CPU object */
#define cpu_force_cli() asm volatile("cli" : : : "memory")

/* An sti that does not update CPU object */
#define cpu_force_sti() asm volatile("sti" : : : "memory")

/* Sets the desired interrupts state for current CPU. Returns previous state. Assumes the current
   CPU is the given CPU. */
static inline bool cpu_set_interrupts_with_cpu(x86_cpu_t *cpu, bool state)
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

/* Relax procedure to use when in a spin-loop */
#define cpu_relax() asm volatile("pause": : :"memory")

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
