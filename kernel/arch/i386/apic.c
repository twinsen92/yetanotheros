/* apic.c - handler for the Advanced Programmable Interrupt Controller */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/init.h>
#include <arch/apic.h>
#include <arch/apic_types.h>
#include <arch/cpu.h>
#include <arch/interrupts.h>
#include <arch/memlayout.h>
#include <arch/mpt.h>
#include <arch/pit.h>

#define mpt_lapic ((volatile lapic_reg_t *)mpt_get_info()->lapic_base)
volatile lapic_reg_t *lapic = NULL;
static uint32_t calibrated_ticr = 0;
static uint64_t bus_freq = 0;

/* Write to LAPIC's register and synchronize on read. */
#define lapicw(reg, val)	\
	lapic[(reg)] = (val);		\
	lapic[(reg)];

#define lapicr(reg) lapic[(reg)]

static void isr_timer_calibration(__unused struct isr_frame *frame)
{
	lapic_eoi();
}

static void isr_timer(__unused struct isr_frame *frame)
{
	/* TODO: Do something more useful here... */
	lapic_eoi();
}

static void isr_spurious(struct isr_frame *frame)
{
	kdprintf("spurious interrupt at %x:%x\n", frame->cs, frame->eip);
	lapic_eoi();
}

static void isr_error(__unused struct isr_frame *frame)
{
	kpanic("error interrupt");
	//lapic_eoi();
}

static void lapic_calibrate_timer_with_pit(void)
{
	uint64_t count;

	/* We need interrupts to be off. */
	if (cpu_get_eflags() & EFLAGS_IF)
		kpanic("need interrupts off when calibrating LAPIC timer");

	/* We temporarily use a dummy ISR. */
	isr_set_handler(INT_IRQ_TIMER, isr_timer_calibration);

	/* Acquire the PIT lock. */
	pit_acquire();
	/* We started with interrupts off, and acquiring the PIT lock might have further modified the
	   cli_stack, so we forcefully enable and disable interrupts here to get the LAPIC timer to
	   work and preserve cli_stack. */
	cpu_force_sti();

	/* Enable the timer and set the divisor. */
	lapicw(LAPIC_REG_TDCR, LAPIC_TIMER_X16);
	lapicw(LAPIC_REG_TIMER, INT_IRQ_TIMER);

	/* Prepare a 1ms countdown in PIT. */
	pit_prepare_counter(1000);
	/* Start the countdown in PIT. */
	pit_start_counter();
	/* Start the LAPIC timer. */
	lapicw(LAPIC_REG_TICR, -1);
	/* Wait for the countdown to finish in PIT. */
	pit_wait_counter();
	/* Stop the LAPIC timer. */
	lapicw(LAPIC_REG_TIMER, LAPIC_MASKED);

	/* Read the LAPIC timer counter register. */
	count = -((int32_t)lapicr(LAPIC_REG_TCCR));

	/* We don't need interrupts anymore. */
	cpu_force_cli();

	/* We used a divisor of 16, so shift the result left by 16. We also waited for 1/1000 of a second
	   so multiply by 1000. We now have the bus frequency in Hz. */
	bus_freq = (count << 4) * 1000;
	/* Calculate the TICR we need to tick with the frequency of TICKS_PER_SECOND. */
	calibrated_ticr = (bus_freq / TICKS_PER_SECOND) >> 4;

	pit_release();

	kdprintf("LAPIC bus frequency: %u\n", (uint32_t)bus_freq);
}

static inline void lapic_enable_calibrated_timer(void)
{
	lapicw(LAPIC_REG_TDCR, LAPIC_TIMER_X16);
	lapicw(LAPIC_REG_TIMER, LAPIC_TIMER_PERIODIC | INT_IRQ_TIMER);
	lapicw(LAPIC_REG_TICR, calibrated_ticr);
}

static void lapic_enable_timer(void)
{
	if (bus_freq != 0)
	{
		lapic_enable_calibrated_timer();
		return;
	}

	/* We do some assumptions that would be safest in initialization phase, with just one CPU. */
	kassert(is_yaos2_initialized() == false);

	lapic_calibrate_timer_with_pit();

	/* Use the real timer ISR now. */
	isr_set_handler(INT_IRQ_TIMER, isr_timer);

	/* Configure the timer with the calibrated TICR value. */
	lapic_enable_calibrated_timer();
}

/* Initializes the local APIC of the current CPU. */
void init_lapic(void)
{
	if (lapic == NULL)
	{
		/* First time configuring a LAPIC. */
		lapic = mpt_get_info()->lapic_base;

		/* TODO: Do a proper ISR registration, like in YAOS1 */
		isr_set_handler(INT_IRQ_TIMER, isr_timer);
		isr_set_handler(INT_IRQ_ERROR, isr_error);
		isr_set_handler(INT_IRQ_SPURIOUS, isr_spurious);
	}

	/* Enable local APIC; set spurious interrupt vector. */
	lapicw(LAPIC_REG_SVR, LAPIC_ENABLE | INT_IRQ_SPURIOUS);

	/* Disable the timer for now. We will enable it after we calibrate it. */
	lapicw(LAPIC_REG_TIMER, LAPIC_TIMER_PERIODIC);

	/* Disable logical interrupt lines. */
	lapicw(LAPIC_REG_LINT0, LAPIC_MASKED);
	lapicw(LAPIC_REG_LINT1, LAPIC_MASKED);

	/* Disable performance counter overflow interrupts on machines that provide that interrupt
	   entry. */
	if(((lapic[LAPIC_REG_VER]>>16) & 0xFF) >= 4)
		lapicw(LAPIC_REG_PCINT, LAPIC_MASKED);

	/* Map error interrupt to IRQ_ERROR. */
	lapicw(LAPIC_REG_ERROR, INT_IRQ_ERROR);

	/* Clear error status register (requires back-to-back writes). */
	lapicw(LAPIC_REG_ESR, 0);
	lapicw(LAPIC_REG_ESR, 0);

	/* Ack any outstanding interrupts. */
	lapicw(LAPIC_REG_EOI, 0);

	/* Send an Init Level De-Assert to synchronise arbitration ID's. */
	lapicw(LAPIC_REG_ICRHI, 0);
	lapicw(LAPIC_REG_ICRLO, LAPIC_ICR_BCAST | LAPIC_ICR_INIT | LAPIC_ICR_LEVEL);

	while(lapic[LAPIC_REG_ICRLO] & LAPIC_ICR_DELIVS);

	/* Enable interrupts on the APIC (but not on the processor). */
	lapicw(LAPIC_REG_TPR, 0);

	lapic_enable_timer();
}

/* Get the current LAPIC ID. Can be used regardless of init_lapic() */
lapic_id_t lapic_get_id(void)
{
	if (lapic)
		return lapic[LAPIC_REG_ID] >> 24;
	else
		return mpt_lapic[LAPIC_REG_ID] >> 24;
}

void lapic_eoi(void)
{
	lapicw(LAPIC_REG_EOI, 0);
}

/* Registers an I/O APIC */
void ioapic_register(__unused paddr_t base, __unused uint8_t id, __unused uint8_t offset,
	__unused uint8_t size)
{

}
