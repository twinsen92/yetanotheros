/* kernel.c - main kernel compilation unit */
#include <kernel/block.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/test.h>
#include <kernel/thread.h>
#include <kernel/vfs.h>
#include <kernel/devices/pci.h>
#include <kernel/fs/devfs.h>
#include <kernel/fs/fat.h>
#include <kernel/exec/elf.h>

/* TODO: Standardize this somehow. */
size_t palloc_get_remaining(void);

/* TODO: Make driver installation more automatic. */
void ata_gen_install(void);

/* TODO: Make this nicer */
void install_com1_cdev(void);
void install_com2_cdev(void);

noreturn kernel_main(void)
{
	kdprintf("Hello, kernel World!\n");

	/* Init non-critical shared subsystems. */
	init_pci();

	/* Install drivers. */
	ata_gen_install();
	install_com1_cdev();
	install_com2_cdev();

	/* Mount the root filesystem. */
	struct block_dev *bd = bdev_get("ata0:0");

	if (bd == NULL)
		kpanic("ata0:0 is missing");

	struct vfs_super *root_fs = fat_create_super(bd);

	vfs_mount("/", root_fs);

	/* Mount devfs */
	struct vfs_super *devfs_root = devfs_get_super();
	vfs_mount("/dev", devfs_root);

	kdprintf("remaining %x bytes (before)\n", palloc_get_remaining());

	//kalloc_test_main();
	//fat_test_main(root_fs);
	for (int i = 0; i < 20; i++)
		exec_user_elf_program("/usr/bin/hello");

	kdprintf("done creating processes\n");
	thread_sleep(1000);
	kdprintf("remaining %x bytes (after)\n", palloc_get_remaining());

	struct file *f = vfs_open("/dev/com1");
	kassert(f != NULL);
	f->write(f, "test write through /dev/com1\n", 29);
	vfs_close(f);

	kdprintf("Bye, kernel World!\n");

	thread_exit();
}
