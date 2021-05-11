/* memlayout.c - helper functions for the virtual memory map and layout */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <arch/memlayout.h>

/*
	Walks the virtual memory map.

	If the physical address is statically mapped to a virtual address, the latter is returned.
	Otherwise, NULL is returned. In general, that means a page walk should be performed.

	Additionally, if panic flag is true, the function panics instead of returning NULL.
*/
vaddr_t vm_map_walk(paddr_t p, bool panic)
{
	for (int i = 0; i < VM_NOF_REGIONS; i++)
	{
		if (vm_map[i].pbase <= p && p < vm_map[i].pbase + vm_map[i].size)
		{
			ptrdiff_t diff = p - vm_map[i].pbase;
			return vm_map[i].vbase + diff;
		}
	}

	if (panic)
		kpanic("vm_map_walk(): physical address could not be resolved");

	return NULL;
}
