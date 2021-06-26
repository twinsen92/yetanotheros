/* heap.c - x86 heap implementation */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/cpu.h>
#include <kernel/debug.h>
#include <kernel/heap.h>
#include <arch/cpu.h>
#include <arch/palloc.h>
#include <arch/memlayout.h>
#include <arch/paging.h>

#define HEAP_ALLOC_MAGIC 0xA110CA73

struct heap_alloc
{
	uint32_t magic1; /* Magic value for detecting buffer overflows. */

	struct heap_alloc *prev; /* Previous allocation or NULL if first. */
	struct heap_alloc *next; /* Next allocation or NULL if last. */
	size_t align_offset_size; /* Offset size due to alignment. */
	size_t size; /* Size of the usable allocation area. */
	bool free; /* true if allocation can be truncated or reused, false otherwise */

	uint32_t magic2; /* Magic value for detecting buffer overflows. */
};

/*
	This is a very simple heap allocator. The allocator keeps track of all allocations as a linked
	list. The items are placed sequentially in the kernel heap region of the virtual memory. Here is
	my attempt to draw it :)

    0               v---heap                      v---first
	[ other memory ][ #1 align_offset_size bytes ][ #1 heap_alloc   ][ #1 size bytes ][ #2 align_...

	 ...offset_size bytes ][ #2 heap_alloc   ][ #2 size bytes ]
	                       ^---last                            ^---heap + cur_size    ^
	                                                               heap + heap_size---|
*/

static bool initialized = false;
static const struct vm_region *heap_region;

static struct cpu_spinlock spinlock; /* Mutex lock for the heap. */
static size_t heap_size; /* Total heap size in bytes. */
static size_t cur_size;
static void *heap; /* Heap base address. */
static struct heap_alloc *first; /* First allocation. */
static struct heap_alloc *last; /* Last allocation. */

static inline bool is_heap(vaddr_t v)
{
	return (heap_region->vbase <= v) && (v < heap_region->vbase + heap_region->size);
}

static inline bool unsafe_check(struct heap_alloc *alloc)
{
	return alloc->magic1 == HEAP_ALLOC_MAGIC && alloc->magic2 == HEAP_ALLOC_MAGIC;
}

static void unsafe_check_all(void)
{
	struct heap_alloc *p;

	p = first;

	while (p)
	{
		kassert(unsafe_check(p));
		p = p->next;
	}

	p = last;

	while (p)
	{
		kassert(unsafe_check(p));
		p = p->next;
	}
}

static void unsafe_grow_heap(size_t new_size)
{
	vaddr_t v, vto;

	/* TODO: Allow kernel heap to free memory. */
	kassert(new_size >= heap_size);

	if (heap + new_size > heap_region->vbase + heap_region->size)
		kpanic("heap exceeded the heap_region");

	/* We do not want to (for whatever, future reason) be holding the physical memory allocator's
	   (palloc.c) lock right now... That could mean a dead-lock */
	if (palloc_lock_held ())
		kpanic("holding palloc lock while using the kernel heap allocator");

	/* Same thing with the paging.c lock. */
	if (kp_lock_held ())
		kpanic("holding kernel page tables write lock while using the kernel heap allocator");

	if (align_to_next_page(heap + heap_size) != align_to_next_page(heap + new_size))
	{
		/* We have to add more pages of memory. */
		v = (vaddr_t)align_to_next_page(heap + heap_size);
		vto = (vaddr_t)align_to_next_page(heap + new_size);

		while (v < vto)
		{
			kp_map(v, palloc());
			v += PAGE_SIZE;
		}
	}

	heap_size = new_size;
}

static void unsafe_truncate_heap(void)
{
	/* Remove trailing, freed allocations. */

	if (!last)
		return;

	while (last && last->free == true)
	{
		cur_size -= last->align_offset_size + sizeof(struct heap_alloc) + last->size;
		last = last->prev;
	}

	if (last == NULL)
		first = NULL;
	else
		last->next = NULL;
}

void init_kernel_heap(const struct vm_region *region)
{
	heap_region = region;
	heap_size = 0;
	/* The heap starts at the beginning of the region. */
	heap = region->vbase;
	first = NULL;
	last = NULL;
	initialized = true;
}

vaddr_t kalloc(int mode, uintptr_t alignment, size_t size)
{
	uintptr_t unaligned;
	size_t offset;
	struct heap_alloc *alloc;
	vaddr_t v;

	/* If we try to acquire the heap spinlock with interrupts off, we might run into a deadlock
	   where one CPU is waiting on the lock with interrupts off and the other is waiting to deliver
	   a flush TLB IPI. */
	if ((cpu_get_eflags() & EFLAGS_IF) == 0 && get_nof_active_cpus() > 1)
		kpanic("kalloc(): called with interrupts off");

	if (mode == HEAP_CONTINUOUS)
		kpanic("kalloc(): HEAP_CONTINUOUS unimplemented");

	/* We can check this because it only changes in initialization. */
	if (!initialized)
		kpanic("kalloc(): heap was not initialized");

	cpu_spinlock_acquire(&spinlock);

	/* TODO: Reuse freed allocations. */
	/* TODO: Reuse heap lost due to alignment. */

#ifdef KERNEL_DEBUG
	unsafe_check_all();
#endif

	/* Calculate the next possible pointer with such alignment. */
	unaligned = (uintptr_t)(heap + cur_size + sizeof(struct heap_alloc));
	offset = alignment - 1 - (size_t)((unaligned + alignment - 1) % alignment);
	v = heap + cur_size + offset + sizeof(struct heap_alloc);

	/* Grow the heap, if necessary. */
	if (heap_size < cur_size + offset + sizeof(struct heap_alloc) + size)
		unsafe_grow_heap(cur_size + offset + sizeof(struct heap_alloc) + size);

	/* Now we can actually access the new memory. */

	/* Build the allocation object. */
	alloc = v - sizeof(struct heap_alloc);
	alloc->magic1 = HEAP_ALLOC_MAGIC;
	alloc->align_offset_size = offset;
	alloc->size = size;
	alloc->free = false;
	alloc->magic2 = HEAP_ALLOC_MAGIC;

	/* Place it within the linked list. */
	if (last)
		last->next = alloc;

	alloc->prev = last;
	alloc->next = NULL;

	if (first == NULL)
		first = alloc;

	last = alloc;

	/* We're almost done. Increase the current heap size. */
	cur_size += offset + sizeof(struct heap_alloc) + size;

#ifdef KERNEL_DEBUG
	unsafe_check_all();
#endif

	cpu_spinlock_release(&spinlock);

	return v;
}

vaddr_t krealloc(__unused vaddr_t v, __unused int mode, __unused uintptr_t alignment, __unused size_t size)
{
	kpanic("krealloc(): unimplemented");
	return v;
}

void kfree(vaddr_t v)
{
	struct heap_alloc *alloc;

	/* We can check this because it only changes in initialization. */
	if (!initialized)
		kpanic("kfree(): heap was not initialized");

	if (!is_heap(v))
		kpanic("kfree(): given address is not in heap region");

	cpu_spinlock_acquire(&spinlock);

	alloc = v - sizeof(struct heap_alloc);

	if (!unsafe_check(alloc))
		kpanic("kfree(): bad address");

	alloc->free = true;

	/* It might be worth truncating the heap if we've free the last allocation or the next one is
	   free as well. */
	if (alloc->next == NULL || alloc->next->free == true)
		unsafe_truncate_heap();

	cpu_spinlock_release(&spinlock);
}

paddr_t ktranslate(__unused vaddr_t v)
{
	/* We can check this because it only changes in initialization. */
	if (!initialized)
		kpanic("ktranslate(): heap was not initialized");

	kpanic("not implemented");
}
