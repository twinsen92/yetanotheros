/* addr.h - address types */
#ifndef _KERNEL_ADDR_H
#define _KERNEL_ADDR_H

#include <kernel/cdefs.h>

typedef void *vaddr_t;
typedef uint32_t vaddr32_t;
typedef void *paddr_t;
typedef uint32_t paddr32_t;

#define to_vaddr(x) ((vaddr_t)(x))
#define to_paddr(x) ((paddr_t)(x))

typedef char symbol_t;

#define get_symbol_vaddr(s) ((vaddr_t)(&(s)))

#endif
