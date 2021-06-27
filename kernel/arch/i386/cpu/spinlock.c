/* cpu/spinlock.c - x86 CPU spinlock implementation */
#include <kernel/cdefs.h>
#include <kernel/cpu.h>
#include <kernel/debug.h>
#include <kernel/thread.h>
#include <arch/cpu.h>
#include <arch/thread.h>

/* TODO: Perhaps use some lighter alternative instead of preempt_disable/push_no_interrupts */

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
	spinlock->num = 0;
	spinlock->cpu = CPU_SPINLOCK_INVALID_CPU;

#ifdef KERNEL_DEBUG
	spinlock->name = name;
	spinlock->tid = TID_INVALID;
	debug_clear_call_stack(&(spinlock->lock_call_stack));
#endif
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
	uint32_t eflags = cpu_get_eflags();

	/* Disable preemption so that the thread we're currently running does not get rescheduled on
	   a different CPU while holding the spinlock. */
	preempt_disable();

	/* Disable interrupts. This way we can avoid weird behaviour when an interrupt handler tries to
	   use the same spinlock. */
	push_no_interrupts();

	if (cpu_spinlock_held(spinlock))
	{
		spinlock->num++;
		goto num_incremented;
	}

	/* If interrupts were enabled before acquire was called, we want to spin with interrupts
	   enabled, so that we can get IPIs and be preempted. Interrupts will be disabled when
	   we acquire the lock. */
	if (eflags & EFLAGS_IF)
		spin_interruptible(spinlock);
	else
		spin_uninterruptible(spinlock);

	cpu_memory_barrier();

	spinlock->num = 1;

	/* Set fields using the CPU object. */
	cpu = cpu_current();

	spinlock->cpu = cpu->num;

#ifdef KERNEL_DEBUG
	if (cpu->thread)
		spinlock->tid = cpu->thread->tid;
	/* Copy the current call stack into the spinlock object. */
	debug_fill_call_stack(&(spinlock->lock_call_stack));
#endif

num_incremented:
	pop_no_interrupts();
}

void cpu_spinlock_release(struct cpu_spinlock *spinlock)
{
	/* Disable interrupts. This way we can avoid weird behaviour when an interrupt handler tries to
	   use the same spinlock. */
	push_no_interrupts();

	if (spinlock->cpu == CPU_SPINLOCK_INVALID_CPU)
		kpanic("cpu_spinlock_release(): called on an unheld spinlock");

	if (!cpu_spinlock_held(spinlock))
		kpanic("cpu_spinlock_release(): called on an unheld spinlock");

	if (--spinlock->num > 0)
		goto num_decremented;

	spinlock->cpu = CPU_SPINLOCK_INVALID_CPU;

#ifdef KERNEL_DEBUG
	/* Clear debug fields. */
	spinlock->tid = TID_INVALID;
	debug_clear_call_stack(&(spinlock->lock_call_stack));
#endif

	cpu_memory_barrier();

	asm volatile ("movl $0, %0" : "+m" (spinlock->locked));

num_decremented:
	pop_no_interrupts();

	/* Since we're no longer holding the lock, enable preemption. */
	preempt_enable();
}

bool cpu_spinlock_held(struct cpu_spinlock *spinlock)
{
	/* Disable interrupts. This way we can avoid weird behaviour when an interrupt handler tries to
	   use the same spinlock. */
	push_no_interrupts();
	bool ret = spinlock->locked && spinlock->cpu == cpu_current()->num;
	pop_no_interrupts();
	return ret;
}
