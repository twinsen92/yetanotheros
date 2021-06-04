/* init.h - generic kernel initialization */
#ifndef _KERNEL_INIT_H
#define _KERNEL_INIT_H

#include <kernel/cdefs.h>

/* This is the state of the kernel initialization. When YAOS2 is initialized, multiple CPUs might
   be running and scheduling threads. */
extern uint32_t yaos2_initialized;

/* Checks whether the basic kernel initialization is complete. */
#define is_yaos2_initialized() (yaos2_initialized > 0)

noreturn kernel_main(void);

#endif
