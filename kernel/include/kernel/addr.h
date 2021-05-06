#ifndef _KERNEL_ADDR_H
#define _KERNEL_ADDR_H

typedef void *vaddr_t;
typedef void *paddr_t;

#define to_vaddr(x) ((vaddr_t)(x))
#define to_paddr(x) ((paddr_t)(x))

typedef char symbol_t;

#define get_symbol_vaddr(s) ((vaddr_t)(&(s)))

#endif
