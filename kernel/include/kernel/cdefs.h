/* cdefs.h - various compiler definitions of types, macros etc. */
#ifndef _KERNEL_CDEFS_H
#define _KERNEL_CDEFS_H

#include <limits.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <sys/cdefs.h>

#define noreturn __attribute__((__noreturn__)) void
#define __unused __attribute__((unused))

#endif
