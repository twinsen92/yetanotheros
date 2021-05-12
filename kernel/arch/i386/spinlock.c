
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

void spinlock_create(spinlock_t *spinlock, const char *name)
{
	spinlock->locked = 0;
	spinlock->name = name;
	spinlock->cpu = -1;
}

void spinlock_acquire(spinlock_t *spinlock)
{
	push_no_interrupts();

	while (asm_xchg(&(spinlock->locked), 1))
		cpu_relax();

	asm_barrier();

	spinlock->cpu = cpu_current()->num;
}

void spinlock_release(spinlock_t *spinlock)
{
	if(!spinlock_held(spinlock))
		kpanic("spinlock_release(): called on an unheld spinlock");

	spinlock->cpu = -1;

	asm_barrier();

	asm volatile ("movl $0, %0" : "+m" (spinlock->locked));

	pop_no_interrupts();
}

bool spinlock_held(spinlock_t *spinlock)
{
	push_no_interrupts();
	bool ret = spinlock->locked && spinlock->cpu == cpu_current()->num;
	pop_no_interrupts();
	return ret;
}
