/* addr.h - address types */
#ifndef _KERNEL_ADDR_H
#define _KERNEL_ADDR_H

#include <kernel/cdefs.h>

/* Virtual address. A pointer type is used because virtual memory is supposed to be usable. */
typedef void *vaddr_t;
typedef uint32_t vaddr32_t;
#define VIRT_NULL NULL

/* Physical address. An integer type is used to catch attempts to dereference. */
typedef uintptr_t paddr_t;
typedef uint32_t paddr32_t;
#define PHYS_NULL 0

#define to_vaddr(x) ((vaddr_t)(x))
#define to_paddr(x) ((paddr_t)(x))

typedef char symbol_t;

#define get_symbol_vaddr(s) ((vaddr_t)(&(s)))

#endif
