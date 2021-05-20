/* apic.h - handler for the Advanced Programmable Interrupt Controller */
#ifndef ARCH_I386_APIC_H
#define ARCH_I386_APIC_H

#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <arch/apic_types.h>

/* Local APIC */

/* Initializes the local APIC of the current CPU. */
void init_lapic(void);

/* Get the current LAPIC ID. Can be used regardless of init_lapic() */
lapic_id_t lapic_get_id(void);

void lapic_eoi(void);

void lapic_ipi(lapic_id_t id, uint8_t vector, uint32_t flags);

void lapic_ipi_wait(void);

void lapic_start_ap(lapic_id_t id, uint16_t entry);

/* I/O APIC */

/* Registers an I/O APIC */
void ioapic_register(paddr_t base, uint8_t id, uint8_t offset, uint8_t size);

#endif
