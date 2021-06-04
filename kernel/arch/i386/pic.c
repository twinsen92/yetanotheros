/* pic.c - handler for the Programmable Interrupt Controller */
#include <arch/kernel/portio.h>

/* PIC ports */

#define PIC1			0x20
#define PIC1_COMMAND	PIC1
#define PIC1_DATA		(PIC1+1)

#define PIC2			0xA0
#define PIC2_COMMAND	PIC2
#define PIC2_DATA		(PIC2+1)

/* Right now we don't want to use the legacy PIC, so we simply disable it. */

void pic_disable(void)
{
	/* Mask all interrupts in both PICs. */
	pio_outb(PIC1_DATA, 0xFF);
	pio_outb(PIC2_DATA, 0xFF);
}
