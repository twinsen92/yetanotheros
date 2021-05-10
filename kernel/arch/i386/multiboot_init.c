/* multiboot_init.c - multiboot x86 init */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/boot/multiboot.h>
#include <arch/init.h>
#include <arch/memlayout.h>
#include <arch/palloc.h>

static const char *mb_mmap_type_texts[] = {
	"invalid",
	"available",
	"reserved",
	"acpi",
	"nvs",
	"badram"
};

static void walk_mmap(struct multiboot_info *info)
{
	const char *text;
	struct multiboot_mmap_entry *cur, *max;

	cur = km_vaddr(info->mmap_addr);
	max = km_vaddr(info->mmap_addr + info->mmap_length);

	while(cur < max)
	{
		text = mb_mmap_type_texts[cur->type];
		kdprintf("PHYS %x + %x %s\n", (uint32_t)cur->addr, (uint32_t)cur->len, text);

		if (false && cur->type == MULTIBOOT_MEMORY_AVAILABLE)
			palloc_add_free_region(cur->addr, cur->addr + cur->len);

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

	/* Waste a few pages to see if palloc works. TODO: Remove this. */
	palloc();
	palloc();
	palloc();

	/* Continue the usual initialization. */
	generic_x86_init();
}
