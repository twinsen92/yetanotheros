/* memlayout.h - layout of the virtual and physical memory */
#ifndef ARCH_I386_MEMLAYOUT_H
#define ARCH_I386_MEMLAYOUT_H

/* Basic assumptions about the kernel layout in virtual and physical memory.  */
#define ASM_KM_PHYS_BASE		0
#define ASM_KM_VIRT_BASE		(0xC0000000 + ASM_KM_PHYS_BASE)
#define ASM_KM_EXEC_PHYS_BASE	0x00100000
#define ASM_KM_EXEC_VIRT_BASE	(ASM_KM_VIRT_BASE + ASM_KM_EXEC_PHYS_BASE)
/* Memory region where devices and the current paging structures will be mapped. */
#define ASM_KM_DEV_VIRT_BASE	0xFE000000
#define ASM_KM_DEV_VIRT_END		0xFFC00000

#ifndef __ASSEMBLER__

#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <arch/paging_types.h>

#define KM_PHYS_BASE		((paddr_t)(ASM_KM_PHYS_BASE))
#define KM_VIRT_BASE		((vaddr_t)(ASM_KM_VIRT_BASE))
#define KM_EXEC_PHYS_BASE	((paddr_t)(ASM_KM_EXEC_PHYS_BASE))
#define KM_EXEC_VIRT_BASE	((vaddr_t)(ASM_KM_EXEC_VIRT_BASE))
#define KM_DEV_VIRT_BASE	((vaddr_t)(ASM_KM_DEV_VIRT_BASE))
#define KM_DEV_VIRT_END		((vaddr_t)(ASM_KM_DEV_VIRT_END))

/* Linker symbols. */
extern symbol_t __kernel_virtual_offset;
extern symbol_t __kernel_low_begin;
extern symbol_t __kernel_low_end;
extern symbol_t __kernel_boot_end;
extern symbol_t __kernel_mem_ro_begin;
extern symbol_t __kernel_mem_ro_end;
extern symbol_t __kernel_mem_rw_begin;
extern symbol_t __kernel_mem_rw_end;
extern symbol_t __kernel_mem_break;

extern symbol_t __kernel_pd;
extern symbol_t __kernel_page_tables;

#define KM_VIRT_LOW_BASE		(get_symbol_vaddr(__kernel_low_begin))
#define KM_VIRT_LOW_END			(get_symbol_vaddr(__kernel_low_end))
#define KM_VIRT_RO_BASE			(get_symbol_vaddr(__kernel_mem_ro_begin))
#define KM_VIRT_RO_END			(get_symbol_vaddr(__kernel_mem_ro_end))
#define KM_VIRT_RW_BASE			(get_symbol_vaddr(__kernel_mem_rw_begin))
#define KM_VIRT_RW_END			(get_symbol_vaddr(__kernel_mem_rw_end))

#define km_paddr(v) (((paddr_t)(v)) - ((uintptr_t)KM_VIRT_BASE))
#define km_vaddr(p) (KM_VIRT_BASE + ((uintptr_t)(p)))

/* Information for the physical memory allocator. */
#define KM_VIRT_END				(get_symbol_vaddr(__kernel_mem_break))
#define KM_FREE_PHYS_BASE		(km_paddr(KM_VIRT_END))

/*
	Virtual space map. Acts as a sort of template for kernel page table construction.

	YAOS2 kernel virtual memory looks like this:

		TYPE				VIRTUAL							PHYSICAL

	Virtual low

						0
						...
						KM_FREE_PHYS_BASE

			Right now virtual low addresses are not used by the kernel. These may be
		mapped by user processes, though.

	Virtual middle

						KM_FREE_PHYS_BASE				KM_FREE_PHYS_BASE
						...								...
						KM_VIRT_BASE					KM_VIRT_BASE

			Virtual middle addresses are mapped 1:1 to physical pages. This area is used
		by the physical page allocator. Since this area might be mapped by user processes,
		the physical page allocator should ensure to use the kernel paging structures
		while allocating.

	Virtual high

						KM_VIRT_BASE					0
						...								...
						KM_VIRT_END						KM_FREE_PHYS_BASE

						KM_VIRT_END
						...									dynamic
						KM_DEV_VIRT_BASE

			Virtual high addresses contain kernel's private data. This includes it's own
		static and dynamic variables, paging structures, user process paging structures,
		etc. This area should be mapped as "global". The map of the dynamic area should be
		propagated to other paging structures, somehow...

						KM_DEV_VIRT_BASE				KM_DEV_VIRT_BASE
						...								...
						KM_DEV_VIRT_END					KM_DEV_VIRT_END

			Memory mapped devices will be mapped in this region.

						KM_DEV_VIRT_END
						...
						0xffffffff

			This region of virtual memory contains the currently used paging structures
		mapped in R/W mode. TODO: This is probably useless...

*/

struct vm_region
{
	/* Virtual address of the base of the region. Must be aligned to page size. */
	vaddr_t vbase;

	/* Physical address of the base of the region. Must be aligned to page size. */
	paddr_t pbase;

	/* Region size, in bytes. */
	size_t size;

	/* Specifies whether the region is always mapped this way. Non-static regions will be mapped
	   on demand only! */
	bool is_static;

	/* Flags to use when mapping the region. */
	pflags_t flags;
};

#define create_vm_map(map)																		\
{																								\
	/* Low (<1M) memory region. Location of BIOS, I/O ports, information tables etc. */			\
	map[0] = (struct vm_region) {																\
		KM_VIRT_LOW_BASE,																		\
		km_paddr(KM_VIRT_LOW_BASE),																\
		km_paddr(KM_VIRT_LOW_END) - km_paddr(KM_VIRT_LOW_BASE),									\
		true,																					\
		PAGE_BIT_RW | PAGE_BIT_GLOBAL															\
	};																							\
	/* Kernel's read-only region. Contains kernel's code and read-only data. */					\
	map[1] = (struct vm_region) {																\
		KM_VIRT_RO_BASE,																		\
		km_paddr(KM_VIRT_RO_BASE),																\
		km_paddr(KM_VIRT_RO_END) - km_paddr(KM_VIRT_RO_BASE),									\
		true,																					\
		PAGE_BIT_GLOBAL																			\
	},																							\
	/* Kernel's R/W region. Contains kernel's non-constant static data. */						\
	map[2] = (struct vm_region) {																\
		KM_VIRT_RW_BASE,																		\
		km_paddr(KM_VIRT_RW_BASE),																\
		km_paddr(KM_VIRT_RW_END) - km_paddr(KM_VIRT_RW_BASE),									\
		true,																					\
		PAGE_BIT_RW | PAGE_BIT_GLOBAL															\
	};																							\
	/* Free physical memory. */																	\
	map[3] = (struct vm_region) {																\
		(vaddr_t)KM_FREE_PHYS_BASE,																\
		KM_FREE_PHYS_BASE,																		\
		/* TODO: This makes us unable to use the last 1GB of physical memory. */				\
		(paddr_t)(KM_VIRT_LOW_BASE) - KM_FREE_PHYS_BASE,										\
		true,																					\
		PAGE_BIT_RW																				\
	};																							\
	/* Dynamic kernel memory region. */															\
	map[4] = (struct vm_region) {																\
		KM_VIRT_END,																			\
		PHYS_NULL,																				\
		KM_DEV_VIRT_BASE - KM_VIRT_END,															\
		false,																					\
		PAGE_BIT_RW | PAGE_BIT_GLOBAL															\
	};																							\
	/* Memory mapped devices region. */															\
	map[5] = (struct vm_region) {																\
		KM_DEV_VIRT_BASE,																		\
		(paddr_t)KM_DEV_VIRT_BASE,																\
		KM_DEV_VIRT_END - KM_DEV_VIRT_BASE,														\
		true,																					\
		PAGE_BIT_RW | PAGE_BIT_GLOBAL															\
	};																							\
}

#define VM_NOF_REGIONS 6

/* Executable region indices, used by early_paging.c to initially map the kernel */

#define VM_KERNEL_EXEC_RO_REGION 1
#define VM_KERNEL_EXEC_RW_REGION 2

/* Other regions */

#define VM_PALLOC_REGION 3
#define VM_DYNAMIC_REGION 4

/* The map itself, defined in early_paging.c */
extern const struct vm_region *vm_map;

/*
	Walks the virtual memory map.

	If the physical address is statically mapped to a virtual address, the latter is returned.
	Otherwise, NULL is returned. In general, that means a page walk should be performed.

	Additionally, if panic flag is true, the function panics instead of returning NULL.

	TODO: Make sure this is used in more places.
*/
vaddr_t vm_map_walk(paddr_t p, bool panic);

/* A reverse to vm_map_walk() */
paddr_t vm_map_rev_walk(vaddr_t v, bool panic);

/* Checks whether the address is known in the virtual memory map. */
bool vm_exists(vaddr_t v);

/* Checks whether the region this address belongs to is statically mapped. Addresses that do not hit
   any region are presumed not static.*/
bool vm_is_static(vaddr_t v);

/* paddr_t version of vm_is_static() */
bool vm_is_static_p(paddr_t p);

/* Get the paging flags that should be used for this address. */
pflags_t vm_get_pflags(vaddr_t v);

#endif

#endif
