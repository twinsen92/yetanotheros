/* spinlock.c - x86 spinlock implementation */
#include <kernel/debug.h>
#include <kernel/interrupts.h>
#include <kernel/spinlock.h>
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

void spinlock_create(struct spinlock *spinlock, const char *name)
{
	spinlock->locked = 0;
	spinlock->name = name;
	spinlock->cpu = SPINLOCK_INVALID_CPU;
}

void spinlock_acquire(struct spinlock *spinlock)
{
	struct x86_cpu *cpu;

	push_no_interrupts();

	while (asm_xchg(&(spinlock->locked), 1))
		cpu_relax();

	asm_barrier();

	cpu = cpu_current_or_null();

	if (cpu)
		spinlock->cpu = cpu->num;
	else
		spinlock->cpu = SPINLOCK_UNKNOWN_CPU;
}

void spinlock_release(struct spinlock *spinlock)
{
	if (spinlock->cpu == SPINLOCK_INVALID_CPU)
		kpanic("spinlock_release(): called on an unheld spinlock");

	/* We only check if we hold the lock if the holding CPU is a known one. This avoids issues in
	   early kernel, where we might have not enumerated CPUs yet. */
	if (spinlock->cpu != SPINLOCK_UNKNOWN_CPU && !spinlock_held(spinlock))
		kpanic("spinlock_release(): called on an unheld spinlock");

	spinlock->cpu = SPINLOCK_INVALID_CPU;

	asm_barrier();

	asm volatile ("movl $0, %0" : "+m" (spinlock->locked));

	pop_no_interrupts();
}

bool spinlock_held(struct spinlock *spinlock)
{
	push_no_interrupts();
	bool ret = spinlock->locked && spinlock->cpu == cpu_current()->num;
	pop_no_interrupts();
	return ret;
}
