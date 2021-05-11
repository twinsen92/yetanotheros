/* apic.c - handler for the Advanced Programmable Interrupt Controller */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <arch/apic.h>
#include <arch/apic_types.h>
#include <arch/interrupts.h>
#include <arch/memlayout.h>
#include <arch/mpt.h>

#define mpt_lapic ((volatile lapic_reg_t *)mpt_get_info()->lapic_base)
volatile lapic_reg_t *lapic = NULL;

/* Write to LAPIC's register and synchronize on read. */
#define lapicw(reg, val)	\
	lapic[(reg)] = (val);		\
	lapic[(reg)];

static void isr_timer(__unused isr_frame_t *frame)
{
	/* TODO: Do something more useful here... */
	lapic_eoi();
}

static void isr_spurious(isr_frame_t *frame)
{
	kdprintf("spurious interrupt at %x:%x\n", frame->cs, frame->eip);
	lapic_eoi();
}

static void isr_error(__unused isr_frame_t *frame)
{
	kpanic("error interrupt");
	//lapic_eoi();
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

	// Enable local APIC; set spurious interrupt vector.
	lapicw(LAPIC_REG_SVR, LAPIC_ENABLE | INT_IRQ_SPURIOUS);

	// The timer repeatedly counts down at bus frequency
	// from lapic[TICR] and then issues an interrupt.
	// If xv6 cared more about precise timekeeping,
	// TICR would be calibrated using an external time source.
	lapicw(LAPIC_REG_TDCR, LAPIC_TIMER_X1);
	lapicw(LAPIC_REG_TIMER, LAPIC_TIMER_PERIODIC | INT_IRQ_TIMER);
	lapicw(LAPIC_REG_TICR, 10000000);

	// Disable logical interrupt lines.
	lapicw(LAPIC_REG_LINT0, LAPIC_MASKED);
	lapicw(LAPIC_REG_LINT0, LAPIC_MASKED);

	// Disable performance counter overflow interrupts
	// on machines that provide that interrupt entry.
	if(((lapic[LAPIC_REG_VER]>>16) & 0xFF) >= 4)
		lapicw(LAPIC_REG_PCINT, LAPIC_MASKED);

	// Map error interrupt to IRQ_ERROR.
	lapicw(LAPIC_REG_ERROR, INT_IRQ_ERROR);

	// Clear error status register (requires back-to-back writes).
	lapicw(LAPIC_REG_ESR, 0);
	lapicw(LAPIC_REG_ESR, 0);

	// Ack any outstanding interrupts.
	lapicw(LAPIC_REG_EOI, 0);

	// Send an Init Level De-Assert to synchronise arbitration ID's.
	lapicw(LAPIC_REG_ICRHI, 0);
	lapicw(LAPIC_REG_ICRLO, LAPIC_ICR_BCAST | LAPIC_ICR_INIT | LAPIC_ICR_LEVEL);

	while(lapic[LAPIC_REG_ICRLO] & LAPIC_ICR_DELIVS);

	// Enable interrupts on the APIC (but not on the processor).
	lapicw(LAPIC_REG_TPR, 0);
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
