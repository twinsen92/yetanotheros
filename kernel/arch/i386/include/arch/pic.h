/* pic.h - handler for the Programmable Interrupt Controller */
#ifndef ARCH_I386_PIC_H
#define ARCH_I386_PIC_H

/* Right now we don't want to use the legacy PIC, so we simply disable it. */

void pic_disable(void);

#endif
