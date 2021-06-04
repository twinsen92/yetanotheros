/* arch/kernel/cpu.h - x86 CPU interface for arch-independent code */
#ifndef ARCH_I386_KERNEL_CPU_H
#define ARCH_I386_KERNEL_CPU_H

/* Get the number of CPUs. */
unsigned int get_nof_cpus(void);

/* Get the number of active CPUs. */
unsigned int get_nof_active_cpus(void);

/* Relax procedure to use when in a spin-loop */
#define cpu_relax() asm volatile("pause": : :"memory")

/* Compile read-write barrier */
#define cpu_memory_barrier() asm volatile ("" : : : "memory")

/* Pushes a "cli" onto the CPU's interrupt flag stack. */
void push_no_interrupts(void);

/* Pops a "cli" from the CPU's interrupt flag stack. */
void pop_no_interrupts(void);

#endif
