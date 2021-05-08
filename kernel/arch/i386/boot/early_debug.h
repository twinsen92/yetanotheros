/* early_debug.h - early debug functions */
#ifndef ARCH_I386_BOOT_EARLY_DEBUG_H
#define ARCH_I386_BOOT_EARLY_DEBUG_H

#include <kernel/cdefs.h>

noreturn early_kassert_failed(void);

#define early_kassert(expr)		\
	if (!expr)					\
		early_kassert_failed()

#endif
