/* kernel.c - main kernel compilation unit */
#include <kernel/block.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/test.h>
#include <kernel/vfs.h>
#include <kernel/devices/pci.h>
#include <kernel/fs/fat.h>

/* TODO: Make driver installation more automatic. */
void ata_gen_install(void);

noreturn kernel_main(void)
{
	kdprintf("Hello, kernel World!\n");

	/* Init non-critical shared subsystems. */
	init_bdev();
	init_pci();

	/* Install drivers. */
	ata_gen_install();

	/* "Mount" the root filesystem. */
	struct block_dev *bd = bdev_get("ata0:0");

	if (bd == NULL)
		kpanic("ata0:0 is missing");

	struct vfs_super *root_fs = fat_create_super(bd);

	vfs_init(root_fs);

	//kalloc_test_main();
	fat_test_main(root_fs);
}
