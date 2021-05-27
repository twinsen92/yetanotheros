/* cpu_spinlock.c - x86 CPU spinlock implementation */
#include <kernel/cdefs.h>
#include <kernel/cpu_spinlock.h>
#include <kernel/debug.h>
#include <kernel/interrupts.h>
#include <arch/cpu.h>

/* Compile read-write barrier */
#define asm_barrier() asm volatile ("" : : : "memory")

static inline int asm_xchg(void *ptr, int x)
{
	__asm__ __volatile__("xchgl %0,%1"
				:"=r" ((int) x)
				:"m" (*(volatile int *)ptr), "0" (x)
				:"memory");
	return x;
}

void cpu_spinlock_create(struct cpu_spinlock *spinlock, const char *name)
{
	spinlock->locked = 0;
	spinlock->name = name;
	spinlock->cpu = CPU_SPINLOCK_INVALID_CPU;
}

static inline void spin_interruptible(struct cpu_spinlock *spinlock)
{
	while (1)
	{
		cpu_force_cli();

		if (asm_xchg(&(spinlock->locked), 1) == 0)
			break;

		cpu_force_sti();

		cpu_relax();
	}
}

static inline void spin_uninterruptible(struct cpu_spinlock *spinlock)
{
	while (asm_xchg(&(spinlock->locked), 1))
		cpu_relax();
}

void cpu_spinlock_acquire(struct cpu_spinlock *spinlock)
{
	struct x86_cpu *cpu;
	bool had_interrupts_enabled = cpu_get_eflags() & EFLAGS_IF;

	push_no_interrupts();

	/* If interrupts were enabled before acquire was called, we want to spin with interrupts
	   enabled, so that we can get IPIs and be preempted. Interrupts will be disabled when
	   we acquire the lock. */
	if (had_interrupts_enabled)
		spin_interruptible(spinlock);
	else
		spin_uninterruptible(spinlock);

	asm_barrier();

	cpu = cpu_current_or_null();

	if (cpu)
		spinlock->cpu = cpu->num;
	else
		spinlock->cpu = CPU_SPINLOCK_UNKNOWN_CPU;
}

void cpu_spinlock_release(struct cpu_spinlock *spinlock)
{
	if (spinlock->cpu == CPU_SPINLOCK_INVALID_CPU)
		kpanic("cpu_spinlock_release(): called on an unheld spinlock");

	/* We only check if we hold the lock if the holding CPU is a known one. This avoids issues in
	   early kernel, where we might have not enumerated CPUs yet. */
	if (spinlock->cpu != CPU_SPINLOCK_UNKNOWN_CPU && !cpu_spinlock_held(spinlock))
		kpanic("cpu_spinlock_release(): called on an unheld spinlock");

	spinlock->cpu = CPU_SPINLOCK_INVALID_CPU;

	asm_barrier();

	asm volatile ("movl $0, %0" : "+m" (spinlock->locked));

	pop_no_interrupts();
}

bool cpu_spinlock_held(struct cpu_spinlock *spinlock)
{
	push_no_interrupts();
	bool ret = spinlock->locked && spinlock->cpu == cpu_current()->num;
	pop_no_interrupts();
	return ret;
}