/* kernel/devices/block.h - base definitons for block devices */
#ifndef _KERNEL_DEVICES_BLOCK_H
#define _KERNEL_DEVICES_BLOCK_H

#include <kernel/cdefs.h>

#define BDEV_MAX_NAME_LENGTH 32

struct block_dev
{
	/* Constant part. */

	char name[BDEV_MAX_NAME_LENGTH]; /* Block device name. */
	size_t block_size; /* Size of a single block, in bytes. */
	uint num_blocks; /* Number of blocks in the device. */

	/* Dynamic part. */

	void *opaque;

	/* Lock the block with number num. Returns an index valid until unlock. */
	uint (*lock)(struct block_dev *dev, uint num);

	/* Unlock the index block. */
	void (*unlock)(struct block_dev *dev, uint index);

	/* Write len bytes to the index block, starting at offset off, from src. */
	void (*write)(struct block_dev *dev, uint index, uint off, const byte *src, uint len);

	/* Read len bytes from the index block, starting at offset off, into dest. */
	void (*read)(struct block_dev *dev, uint index, uint off, byte *dest, uint len);
};

/* Init the registry of block devices. */
void init_bdev(void);

/* Add a block device. */
void bdev_add(struct block_dev *dev);

/* Get a block device by name. */
struct block_dev *bdev_get(const char *name);

#endif
