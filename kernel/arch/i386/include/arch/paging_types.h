/* cpu/paging_types.h - x86 paging types */
#ifndef ARCH_I386_PAGING_TYPES_H
#define ARCH_I386_PAGING_TYPES_H

/* Some bit constants for flags fields. */

#define PAGE_BIT_PRESENT	0x001
#define PAGE_BIT_RW			0x002
#define PAGE_BIT_USER		0x004
#define PAGE_BIT_GLOBAL		0x100

#define PAGE_RW_PRESENT		0x003

/* Control register paging related bits. */

#define CR0_PG				0x80000000 /* Paging enable */
#define CR0_WP				0x00010000 /* Write protection enable bit */
#define CR4_PGE				0x00000080 /* Global page enable */

#ifndef __ASSEMBLER__

#include <kernel/addr.h>
#include <kernel/cdefs.h>

/* Page table directory type. */
typedef uint32_t pde_t;

/* Page table type. */
typedef uint32_t pte_t;

/* Page flags type. */
typedef uint32_t pflags_t;

#define pte_construct(addr, flags) ((pte_t)(((uint32_t)(addr)) & 0xfffff000) | (((uint32_t)(flags)) & 0x00000fff))
#define pde_construct(addr, flags) ((pde_t)(((uint32_t)(addr)) & 0xfffff000) | (((uint32_t)(flags)) & 0x00000fff))
#define pte_get_paddr(pte) ((paddr_t)(((pte_t)(pte)) & 0xfffff000))
#define pde_get_paddr(pde) ((paddr_t)(((pde_t)(pde)) & 0xfffff000))
#define pte_get_flags(pte) ((paddr_t)(((pte_t)(pte)) & 0x00000fff))
#define pde_get_flags(pde) ((paddr_t)(((pde_t)(pde)) & 0x00000fff))
#define pte_has_flags(pte, flags) (((pte_t)(pte)) & ((uint32_t)(flags)) == ((uint32_t)(flags)))
#define pde_has_flags(pde, flags) (((pde_t)(pde)) & ((uint32_t)(flags)) == ((uint32_t)(flags)))

#endif

#endif
