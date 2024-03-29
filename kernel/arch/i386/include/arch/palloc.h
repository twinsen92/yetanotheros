/* palloc.h - physical memory allocator */
#ifndef ARCH_I386_PALLOC_H
#define ARCH_I386_PALLOC_H

#include <kernel/addr.h>
#include <kernel/cdefs.h>

/* Initializes the physical memory allocator. This can be called multiple times. */
void init_palloc(void);

/* Add a memory region to palloc. Both addresses must be aligned to page boundaries. */
void palloc_add_free_region(paddr_t from, paddr_t to);

/* Get the size of the page returned by palloc() */
uint palloc_get_granularity(void);

/* Get the size of the remaining (free) physical memory. */
size_t palloc_get_remaining(void);

/* Get the next free physical memory page or PHYS_NULL if none are available. */
paddr_t palloc(void);

/* Free the given physical memory page. */
void pfree(paddr_t p);

/* Translate physical address to virtual address.
   NOTE: This does not walk the page tables. This function assumes it has been called with kernel
   page tables in CR3, and does a light-weight calculation based on palloc's virtual mem region. */
vaddr_t ptranslate(paddr_t p);

/* Check if we're holding the physical memory allocator's lock. This is used to avoid dead-locks. */
bool palloc_lock_held(void);

#endif
