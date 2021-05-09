/* cpu.h - x86 cpu state information */
#ifndef ARCH_I386_CPU_H
#define ARCH_I386_CPU_H

#include <kernel/cdefs.h>
#include <arch/gdt.h>
#include <arch/seg_types.h>

#define X86_CPU_MAGIC 0x86

typedef struct
{
	int num;
	int magic;
	/* Interrupts */
	bool int_enabled; /* Interrupts state when cli_stack was 0. */
	int cli_stack; /* Number of cli push operations. */
	/* Segmentation */
	dtr_t gdtr;
	seg_t gdt[YAOS2_GDT_NOF_ENTRIES];
	volatile tss_t tss;
}
x86_cpu_t;

/* Initializes the one CPU object we have. Disables interrupts. */
void init_cpu(void);

/* Gets the current CPU object. */
x86_cpu_t *cpu_current(void);

/* A cli that does not update CPU object */
#define cpu_force_cli() asm volatile("cli" : : : "memory")

/* An sti that does not update CPU object */
#define cpu_force_sti() asm volatile("sti" : : : "memory")

/* Sets the desired interrupts state for current CPU. Returns previous state. */
static inline bool cpu_set_interrupts(bool state)
{
	x86_cpu_t *cpu = cpu_current();
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
