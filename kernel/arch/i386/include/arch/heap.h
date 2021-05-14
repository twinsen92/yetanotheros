/* heap.h - kernel heap interface */
#ifndef ARCH_I386_HEAP_H
#define ARCH_I386_HEAP_H

#include <arch/memlayout.h>

void init_kernel_heap(const vm_region_t *region);

#endif
