/* init.h - generic kernel initialization */
#ifndef _KERNEL_INIT_H
#define _KERNEL_INIT_H

#include <kernel/cdefs.h>

/* Defined at boot, this is the state of the kernel initialization. TODO: Describe further. */
extern uint32_t yaos2_initialized;

/* Checks whether the basic kernel initialization is complete. TODO: Describe further. */
#define is_yaos2_initialized() (yaos2_initialized > 0)

noreturn kernel_main(void);

#endif
