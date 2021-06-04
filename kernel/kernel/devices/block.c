/* kernel/devices/block.c - definitons for block devices */
#include <kernel/cdefs.h>
#include <kernel/cpu.h>
#include <kernel/debug.h>
#include <kernel/utils.h>
#include <kernel/devices/block.h>

/* TODO: Use a linked list. */

static atomic_bool bdev_initialized = false;
static struct cpu_spinlock bdev_spinlock;
#define MAX_BLOCK_DEVICES 8
static struct block_dev *block_devices[MAX_BLOCK_DEVICES];

/* Init the registry of block devices. */
void init_bdev(void)
{
	cpu_spinlock_create(&bdev_spinlock, "block device registry spinlock");

	for (int i = 0; i < MAX_BLOCK_DEVICES; i++)
		block_devices[i] = NULL;

	atomic_store(&bdev_initialized, true);
}

/* Add a block device. */
void bdev_add(struct block_dev *dev)
{
	int i;

	if (!atomic_load(&bdev_initialized))
		kpanic("bdev_add(): not initialized");

	cpu_spinlock_acquire(&bdev_spinlock);

	for (i = 0; i < MAX_BLOCK_DEVICES; i++)
	{
		if (block_devices[i] == NULL)
		{
			block_devices[i] = dev;
			break;
		}
	}

	if (i == MAX_BLOCK_DEVICES)
		kpanic("bdev_add(): max devices exceeded");

	cpu_spinlock_release(&bdev_spinlock);
}

/* Get a block device by name. */
struct block_dev *bdev_get(const char *name)
{
	struct block_dev *dev = NULL;
	int len, len_dev;

	if (!atomic_load(&bdev_initialized))
		kpanic("bdev_add(): not initialized");

	len = kstrlen(name);

	if (len >= BDEV_MAX_NAME_LENGTH)
		kpanic("bdev_get(): name too long");

	cpu_spinlock_acquire(&bdev_spinlock);

	for (int i = 0; i < MAX_BLOCK_DEVICES; i++)
	{
		if (block_devices[i] == NULL)
			continue;

		len_dev = kstrlen(block_devices[i]->name);

		if (len != len_dev)
			continue;

		if (kstrncmp(block_devices[i]->name, name, len))
		{
			dev = block_devices[i];
			break;
		}
	}

	cpu_spinlock_release(&bdev_spinlock);

	return dev;
}
