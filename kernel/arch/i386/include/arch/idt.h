/* idt.h - x86 IDT structures and declarations */
#ifndef ARCH_I386_IDT_H
#define ARCH_I386_IDT_H

#include <kernel/cdefs.h>
#include <arch/seg_types.h>
#include <arch/interrupts.h>

/* Segement bits. All bits are shifted 32 bits to the right. */

#define IDTE_TASK_32 0x500
#define IDTE_INTERRUPT_16 0x600
#define IDTE_TRAP_16 0x700
#define IDTE_INTERRUPT_32 0xE00
#define IDTE_TRAP_32 0xF00

/* Macros and inlines. */

static inline seg_t idte_construct(uint32_t offset, uint16_t selector, uint32_t flags) {
	seg_t seg = 0;

	/* Modify upper 32 bits. */
	seg |= flags;
	seg |= offset & 0xFFFF0000; /* offset bits 31:16 */

	seg <<= 32;

	/* Modify lower 32 bits. */
	seg |= selector << 16; /* selector bits 15:0 */
	seg |= offset & 0x0000FFFF; /* offset bits 15:0 */

	return seg;
}

#define IDT_NOF_ENTRIES ISR_MAX

/* YAOS2 stuff */

/* Initializes the IDT. */
void init_idt(void);

/* Loads the IDT pointer into the current CPU. */
void load_idt(void);

/* Sets an entry in the IDT. */
void idt_set_entry(int_no_t int_no, uint32_t offset, uint16_t selector, uint32_t flags);

#endif
