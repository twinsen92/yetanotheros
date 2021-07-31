/* addr.h - address types */
#ifndef _KERNEL_ADDR_H
#define _KERNEL_ADDR_H

#include <kernel/cdefs.h>

/* Virtual address. A pointer type is used because virtual memory is supposed to be usable. */
typedef void *vaddr_t;
typedef ptrdiff_t vaddrdiff_t;
typedef uint32_t vaddr32_t;

/* Physical address. An integer type is used to catch attempts to dereference. */
typedef uintptr_t paddr_t;
typedef uint32_t paddr32_t;
#define PHYS_NULL 0

/* Verbatim converters for vaddr_t/paddr_t */
#define to_vaddr(x) ((vaddr_t)(x))
#define to_paddr(x) ((paddr_t)(x))

/* Symbol "type". A symbol is not guaranteed to point at actual data, but we use it as sort of
   markers in virtual memory. */
typedef char symbol_t;

/* Gets the location of a symbol in virtual memory. */
#define get_symbol_vaddr(s) ((vaddr_t)(&(s)))

#endif
