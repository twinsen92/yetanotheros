/* kernel/paging.h - arch-independent memory paging definitions and utilities */
#ifndef _KERNEL_PAGING_H
#define _KERNEL_PAGING_H

#include <kernel/cdefs.h>

/* Kernel-wide memory page size definiton. This has to be obeyed in the whole kernel. */
#define PAGE_SIZE 4096

#define get_sub_page_addr(p) (((uintptr_t)(p)) & (PAGE_SIZE - 1))

#define align_to_next_page(x) ((uintptr_t)(x + PAGE_SIZE - 1) & ~((uintptr_t)PAGE_SIZE - 1))
#define mask_to_page(x) ((uintptr_t)(x) & ~((uintptr_t)PAGE_SIZE - 1))

#define is_aligned_to_page_size(x) (((uintptr_t)(x)) == mask_to_page(x))

#endif
