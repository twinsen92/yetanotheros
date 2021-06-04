/* multiboot_init.c - multiboot x86 init */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/boot/multiboot.h>
#include <arch/init.h>
#include <arch/memlayout.h>
#include <arch/palloc.h>
#include <arch/cpu/paging.h>

static const char *mb_mmap_type_texts[] = {
	"invalid",
	"available",
	"reserved",
	"acpi",
	"nvs",
	"badram"
};

/* Walks the Multiboot memory map structures and calls palloc_add_free_region() */
static void walk_mmap(struct multiboot_info *info)
{
	const char *text;
	struct multiboot_mmap_entry *cur, *max;
	paddr_t from, to;

	cur = km_vaddr(info->mmap_addr);
	max = km_vaddr(info->mmap_addr + info->mmap_length);

	while(cur < max)
	{
		text = mb_mmap_type_texts[cur->type];
		kdprintf("PHYS %x + %x %s\n", (uint32_t)cur->addr, (uint32_t)cur->len, text);

		if (cur->type == MULTIBOOT_MEMORY_AVAILABLE)
		{
			/* Align the addresses to page boundaries */
			from = align_to_next_page(cur->addr);
			to = mask_to_page(cur->addr + cur->len);
			palloc_add_free_region(from, to);
		}

		cur = ((void*)cur) + cur->size + sizeof(cur->size);
	}
}

noreturn multiboot_x86_init(struct multiboot_info *info, uint32_t magic)
{
	/* Ensure the kernel has been loaded by multiboot. */
	kassert(magic == MULTIBOOT_BOOTLOADER_MAGIC);

	/* Walk the memory map to initialize palloc. */
	kassert(mb_has_simple_mmap(info));
	init_palloc();
	walk_mmap(info);

	/* Continue the usual initialization. */
	generic_x86_init();
}
