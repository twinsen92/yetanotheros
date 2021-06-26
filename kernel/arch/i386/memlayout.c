/* memlayout.c - helper functions for the virtual memory map and layout */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <arch/memlayout.h>
#include <arch/paging_types.h>

static inline const struct vm_region *get_region_v(vaddr_t v)
{
	for (int i = 0; i < VM_NOF_REGIONS; i++)
	{
		if (vm_map[i].vbase <= v && v < vm_map[i].vbase + vm_map[i].size)
		{
			return &(vm_map[i]);
		}
	}

	return NULL;
}

static inline const struct vm_region *get_region_p(paddr_t p)
{
	for (int i = 0; i < VM_NOF_REGIONS; i++)
	{
		if (vm_map[i].pbase <= p && p < vm_map[i].pbase + vm_map[i].size)
		{
			return &(vm_map[i]);
		}
	}

	return NULL;
}

/*
	Walks the virtual memory map.

	If the physical address is statically mapped to a virtual address, the latter is returned.
	Otherwise, NULL is returned. In general, that means a page walk should be performed.

	Additionally, if panic flag is true, the function panics instead of returning NULL.
*/
vaddr_t vm_map_walk(paddr_t p, bool panic)
{
	const struct vm_region *region = get_region_p(p);

	if (region)
	{
		ptrdiff_t diff = p - region->pbase;
		return region->vbase + diff;
	}

	if (panic)
		kpanic("vm_map_walk(): physical address could not be resolved");

	return NULL;
}

/* A reverse to vm_map_walk() */
paddr_t vm_map_rev_walk(vaddr_t v, bool panic)
{
	const struct vm_region *region = get_region_v(v);

	if (region)
	{
		ptrdiff_t diff = v - region->vbase;
		return region->pbase + diff;
	}

	if (panic)
		kpanic("vm_map_walk(): physical address could not be resolved");

	return PHYS_NULL;
}

/* Checks whether the address is known in the virtual memory map. */
bool vm_exists(vaddr_t v)
{
	const struct vm_region *region = get_region_v(v);
	return region != NULL;
}

/* Checks whether the region this address belongs to is statically mapped. */
bool vm_is_static(vaddr_t v)
{
	const struct vm_region *region = get_region_v(v);

	if (region)
		return region->flags & VM_BIT_STATIC;

	return false;
}

/* paddr_t version of vm_is_static() */
bool vm_is_static_p(paddr_t p)
{
	const struct vm_region *region = get_region_p(p);

	if (region)
		return region->flags & VM_BIT_STATIC;

	return false;
}

/* Get the paging flags that should be used for this address. */
pflags_t vm_get_pflags(vaddr_t v)
{
	const struct vm_region *region = get_region_v(v);

	if (region)
		return region->pflags;

	return 0;
}
