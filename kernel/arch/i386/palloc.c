/* palloc.c - physical memory allocator */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/init.h>
#include <kernel/spinlock.h>
#include <arch/memlayout.h>
#include <arch/paging.h>
#include <arch/palloc.h>

#define PALLOC_PAGE_MAGIC 0x12345678

typedef struct _page_t
{
	uint32_t magic;
	struct _page_t *prev;
	struct _page_t *next;
} page_t;

static bool initialized = false;
static spinlock_t spinlock;
static page_t *first_free_page;
static const vm_region_t *vm_region;

/* Checks whether the given page is mappable in the palloc's virtual memory region. */
static inline bool is_mappable(paddr_t p)
{
	return (vm_region->pbase <= p) && (p < vm_region->pbase + vm_region->size);
}

/* Translates a virtual address to physical address. This assumes the address is within palloc's
   virtual memory region. */
static inline paddr_t vtranslate(vaddr_t v)
{
	kassert((vm_region->vbase <= v) && (v < vm_region->vbase + vm_region->size));
	return vm_region->pbase + (((uintptr_t)v) - ((uintptr_t)vm_region->vbase));
}

/* Initializes the physical memory allocator. This can be called multiple times. */
void init_palloc(void)
{
	if (initialized)
		return;

	kassert(is_yaos2_initialized() == false);

	first_free_page = NULL;
	vm_region = vm_map + VM_PALLOC_REGION;
	spinlock_create(&spinlock, "palloc");
	initialized = true;
}

/* Add a memory region to palloc. */
void palloc_add_free_region(paddr_t from, paddr_t to)
{
	kassert(is_yaos2_initialized() == false);
	kassert(is_aligned_to_page_size(from));
	kassert(is_aligned_to_page_size(to));

	for(; from < to; from += PAGE_SIZE)
	{
		if (!is_mappable(from))
			continue;

		/* Check if the page has been already added to palloc by looking at the magic field.
		   TODO: This is bad. Should be reworked.*/
		page_t *page = ptranslate(from);

		if (page->magic == PALLOC_PAGE_MAGIC)
			continue;

		pfree(from);
	}
}

/* Get the size of the page returned by palloc() */
uint32_t palloc_get_granularity(void)
{
	return PAGE_SIZE;
}

/* Get the next free physical memory page or PHYS_NULL if none are available. */
paddr_t palloc(void)
{
	paddr_t result = PHYS_NULL;

	spinlock_acquire(&spinlock);

	/* We need this to be called with kernel page tables. Otherwise we might read and/or write
	   to pages containing the user program. */
	if (!is_using_kernel_page_tables())
		kpanic("palloc(): called with non-kernel page tables");

	page_t *page = first_free_page;

	if (page == NULL)
		goto _palloc_exit;

	if (page->magic != PALLOC_PAGE_MAGIC)
		kpanic("palloc(): unallocated page has been tampered with");

	first_free_page = page->next;
	if (first_free_page != NULL)
		first_free_page->prev = NULL;

	result = vtranslate(page);

_palloc_exit:
	spinlock_release(&spinlock);

	return result;
}

/* Free the given physical memory page. */
void pfree(paddr_t p)
{
	if (!is_mappable(p))
		kpanic("pfree(): attempted to free a page from outside of the palloc region");

	spinlock_acquire(&spinlock);

	/* We need this to be called with kernel page tables. Otherwise we might read and/or write
	   to pages containing the user program. */
	if (!is_using_kernel_page_tables())
		kpanic("pfree(): called with non-kernel page tables");

	page_t *page = ptranslate(p);

	/* Page won't be NULL, because we already checked if it is mappable. */

	page->magic = PALLOC_PAGE_MAGIC;
	page->prev = NULL;
	page->next = first_free_page;
	if (first_free_page != NULL)
		first_free_page->prev = page;
	first_free_page = page;

	spinlock_release(&spinlock);
}

/* Translate physical address to virtual address.
   NOTE: This does not walk the page tables. This function assumes it has been called with kernel
   page tables in CR3, and does a light-weight calculation based on palloc's virtual mem region. */
vaddr_t ptranslate(paddr_t p)
{
	if (!is_mappable(p))
		return NULL;
	return vm_region->vbase + (p - vm_region->pbase);
}
