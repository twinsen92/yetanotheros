/* heap.h - kernel heap interface */
#ifndef _KERNEL_HEAP_H
#define _KERNEL_HEAP_H

#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/utils.h>

/* Normal allocation, may be fragmented on physical memory. */
#define HEAP_NORMAL 1

/* Unfragmented allocation. This kind of allocation is guaranteed to be continuous on physical
   memory. */
#define HEAP_CONTINUOUS 2

#define HEAP_NO_ALIGN 1

/* Allocate a memory region in kernel heap. */
vaddr_t kalloc(int mode, uintptr_t alignment, size_t size);

/* Allocate a zero-filled memory region in kernel heap. */
static inline vaddr_t kzalloc(int mode, uintptr_t alignment, size_t size)
{
	vaddr_t v = kalloc(mode, alignment, size);
	kmemset(v, 0, size);
	return v;
}
/* Allocate an unaligned memory region in kernel heap. */
static inline vaddr_t kualloc(int mode, size_t size)
{
	return kalloc(mode, HEAP_NO_ALIGN, size);
}

/* Allocate an unaligned zero-filled memory region in kernel heap. */
static inline vaddr_t kzualloc(int mode, size_t size)
{
	vaddr_t v = kualloc(mode, size);
	kmemset(v, 0, size);
	return v;
}

vaddr_t krealloc(vaddr_t v, int mode, uintptr_t alignment, size_t size);
void kfree(vaddr_t v);
paddr_t ktranslate(vaddr_t v);

#endif
