/* cdefs.h - various compiler definitions of types, macros etc. */
#ifndef _KERNEL_CDEFS_H
#define _KERNEL_CDEFS_H

#include <limits.h>

/* For some reason, the Eclipse CDT parser has issues with stdatomic.h. So we define some dummy
   types to get it to behave. */
#ifdef __CDT_PARSER__
typedef _Bool atomic_bool;
typedef int atomic_int;
typedef unsigned int atomic_uint;
typedef __UINT_FAST64_TYPE__ atomic_uint_fast64_t;
#else
#include <stdatomic.h>
#endif

#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <sys/cdefs.h>

#define noreturn __attribute__((__noreturn__)) void
#define __unused __attribute__((unused))
#define packed_struct struct __attribute__((packed))
#define packed_union union __attribute__((packed))

typedef unsigned int uint;
typedef unsigned char byte;
typedef long ssize_t;

#endif
