/* idt.c - x86 IDT handler code */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/init.h>
#include <arch/idt.h>
#include <arch/interrupts.h>
#include <arch/seg_types.h>
#include <arch/selectors.h>

/* TODO: Protect with a lock. */
static seg_t idt[IDT_NOF_ENTRIES];
static dtr_t idtr = { sizeof(idt), (uint32_t)&idt };

/* Initializes the IDT. */
void init_idt(void)
{
	kassert(is_yaos2_initialized() == false);

	/* We assume 32-bit here... */
	kassert(sizeof(vaddr_t) == sizeof(uint32_t));

	for (int i = 0; i < IDT_NOF_ENTRIES; i++)
		idt[i] = idte_construct((uint32_t)isr_stubs[i], KERNEL_CODE_SELECTOR,
			SEG_BIT_PRESENT | IDTE_INTERRUPT_32 | SEG_BIT_RING(0));
}

/* Loads the IDT pointer into the current CPU. */
void load_idt(void)
{
	asm_ldtr("idt", &idtr);
}

/* Sets an entry in the IDT. */
void idt_set_entry(int_no_t int_no, uint32_t offset, uint16_t selector, uint32_t flags)
{
	kassert(int_no < IDT_NOF_ENTRIES);
	idt[int_no] = idte_construct(offset, selector, flags);
}
