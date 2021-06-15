/* kernel.c - main kernel compilation unit */
#include <kernel/block.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/test.h>
#include <kernel/utils.h>
#include <kernel/vfs.h>
#include <kernel/devices/pci.h>
#include <kernel/fs/fat.h>

/* TODO: Make driver installation more automatic. */
void ata_gen_install(void);

noreturn kernel_main(void)
{
	char buf[16];
	uint num;

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

	/* Read a file and print it's contents to the debug output. */
	kmemset(buf, 0, sizeof(buf));

	struct file *f = vfs_open("/something/ffffff/test.txt");

	while (1)
	{
		num = f->read(f, buf, sizeof(buf) - 1);

		if (num == 0)
			break;

		buf[num] = 0;

		kdprintf("%s", buf);
	}

	//kalloc_test_main();
	while(1);
}
