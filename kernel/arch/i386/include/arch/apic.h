/* apic.h - handler for the Advanced Programmable Interrupt Controller */
#ifndef ARCH_I386_APIC_H
#define ARCH_I386_APIC_H

#include <kernel/addr.h>
#include <kernel/cdefs.h>

/* Local APIC */

/* Initializes the local APIC of the current CPU. */
void init_lapic(void);

/* I/O APIC */

/* Registers an I/O APIC */
void ioapic_register(paddr_t base, uint8_t id, uint8_t offset, uint8_t size);

#endif
