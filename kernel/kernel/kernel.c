/* kernel.c - main kernel compilation unit */
#include <kernel/block.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/test.h>
#include <kernel/devices/pci.h>

/* TODO: Make driver installation more automatic. */
void ata_gen_install(void);

byte zero_sector[512];

noreturn kernel_main(void)
{
	kdprintf("Hello, kernel World!\n");

	/* Init non-critical shared subsystems. */
	init_bdev();
	init_pci();

	/* Install drivers. */
	ata_gen_install();

	struct block_dev *bd = bdev_get("ata0");

	uint idx = bd->lock(bd, 0);
	bd->read(bd, idx, 0, zero_sector, 512);
	bd->unlock(bd, idx);

	//kalloc_test_main();
	while(1);
}
