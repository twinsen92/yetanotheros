/* heap.h - kernel heap interface */
#ifndef _KERNEL_HEAP_H
#define _KERNEL_HEAP_H

#include <kernel/addr.h>
#include <kernel/cdefs.h>

/* Normal allocation, may be fragmented on physical memory. */
#define HEAP_NORMAL 1

/* Unfragmented allocation. This kind of allocation is guaranteed to be continuous on physical
   memory. */
#define HEAP_CONTINUOUS 2

vaddr_t kalloc(int mode, uintptr_t alignment, size_t size);
void kfree(vaddr_t v);
paddr_t ktranslate(vaddr_t v);

#endif
