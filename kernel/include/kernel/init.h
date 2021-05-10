/* init.h - generic kernel initialization */
#ifndef _KERNEL_INIT_H
#define _KERNEL_INIT_H

#include <kernel/cdefs.h>

extern uint32_t yaos2_initialized;

#define is_yaos2_initialized() (yaos2_initialized > 0)

noreturn kernel_main(void);

#endif
