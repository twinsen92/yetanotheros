/* apic.c - handler for the Advanced Programmable Interrupt Controller */
#include <kernel/addr.h>
#include <kernel/cdefs.h>

/* Initializes the local APIC of the current CPU. */
void init_lapic(void)
{

}

/* Registers an I/O APIC */
void ioapic_register(paddr_t base, uint8_t id, uint8_t offset, uint8_t size)
{

}
