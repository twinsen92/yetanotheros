/* boot_layout.h - boot-time memory layout of the kernel executable */
#ifndef ARCH_I386_BOOT_LAYOUT_H
#define ARCH_I386_BOOT_LAYOUT_H

#include <kernel/addr.h>
#include <kernel/cdefs.h>

extern symbol_t __kernel_virtual_offset;
#define KERNEL_BASE_VADDR (get_symbol_vaddr(__kernel_virtual_offset))
extern symbol_t __kernel_boot_end;
extern symbol_t __kernel_low_begin;
extern symbol_t __kernel_low_end;
extern symbol_t __kernel_mem_ro_begin;
extern symbol_t __kernel_mem_ro_end;
extern symbol_t __kernel_mem_rw_begin;
extern symbol_t __kernel_mem_rw_end;
extern symbol_t __kernel_mem_break;

extern symbol_t __kernel_pd;
extern symbol_t __kernel_page_tables;

/* Get the virtual address of a low (<1M) physical memory region address. */
#define get_low_vaddr(p) (KERNEL_BASE_VADDR + ((uint32_t)(p)))

#endif
