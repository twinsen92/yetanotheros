/* libc/stdlib/heap.c - stdlib.h heap functions implementation */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HEAP_ALLOC_MAGIC 0xA110CA73

struct heap_alloc
{
	unsigned int magic1; /* Magic value for detecting buffer overflows. */

	struct heap_alloc *prev; /* Previous allocation or NULL if first. */
	struct heap_alloc *next; /* Next allocation or NULL if last. */
	size_t size; /* Size of the usable allocation area. */
	int free; /* 1 if allocation can be truncated or reused, 0 otherwise */

	unsigned int magic2; /* Magic value for detecting buffer overflows. */
};

static size_t heap_size = 0; /* Total heap size in bytes. */
static size_t cur_size = 0;
static void *heap = NULL; /* Heap base address. */
static struct heap_alloc *first = NULL; /* First allocation. */
static struct heap_alloc *last = NULL; /* Last allocation. */

static inline int unsafe_check(struct heap_alloc *alloc)
{
	return alloc->magic1 == HEAP_ALLOC_MAGIC && alloc->magic2 == HEAP_ALLOC_MAGIC;
}

static void unsafe_check_all(void)
{
	struct heap_alloc *p;

	p = first;

	while (p)
	{
		assert(unsafe_check(p));
		p = p->next;
	}

	p = last;

	while (p)
	{
		assert(unsafe_check(p));
		p = p->next;
	}
}

static inline void lazily_init_heap(void)
{
	if (heap == NULL)
		heap = sbrk(0);
}

static void unsafe_truncate_heap(void)
{
	/* Remove trailing, freed allocations. */

	if (!last)
		return;

	while (last && last->free)
	{
		cur_size -= sizeof(struct heap_alloc) + last->size;
		last = last->prev;
	}

	if (last == NULL)
		first = NULL;
	else
		last->next = NULL;
}

void *calloc(size_t num, size_t size)
{
	void *ptr = malloc(num * size);

	if (ptr != NULL)
		memset(ptr, 0, num * size);

	return ptr;
}

void *malloc(size_t size)
{
	void *ret;
	size_t new_size;
	struct heap_alloc *alloc;

	lazily_init_heap();

	/* TODO: Reuse freed allocations. */

#ifdef LIBC_DEBUG
	unsafe_check_all();
#endif

	ret = heap + cur_size + sizeof(struct heap_alloc);
	new_size = cur_size + sizeof(struct heap_alloc) + size;

	/* Grow the heap, if necessary. */
	if (heap_size < new_size)
	{
		if (brk(heap + new_size))
			return NULL;
		heap_size = new_size;
	}

	/* Now we can actually access the new memory. */

	/* Build the allocation object. */
	alloc = ret - sizeof(struct heap_alloc);
	alloc->magic1 = HEAP_ALLOC_MAGIC;
	alloc->size = size;
	alloc->free = 0;
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
	cur_size += sizeof(struct heap_alloc) + size;

#ifdef LIBC_DEBUG
	unsafe_check_all();
#endif

	return ret;
}

void free(void *ptr)
{
	struct heap_alloc *alloc;

	alloc = ptr - sizeof(struct heap_alloc);
	alloc->free = 1;

	/* It might be worth truncating the heap if we've free the last allocation or the next one is
	   free as well. */
	if (alloc->next == NULL || alloc->next->free)
		unsafe_truncate_heap();
}
