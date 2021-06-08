/* kernel/block/mbr.c - MBR partition table parser */
#include <kernel/block.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/heap.h>
#include <kernel/printf.h>
#include <kernel/utils.h>

/* TODO: Endianness? */
#define MBR_BOOT_SIGNATURE 0xaa55
#define MBR_PART_PRESENT 0x80

#define MBR_FAT32 0x0b
#define MBR_FAT32_LBA 0x0c

#define MBR_SECTOR_SIZE 512

packed_struct mbr_partition
{
	uint8_t status;
	uint8_t head;
	uint8_t cylinder_sector;
	uint8_t cylinder;
	uint8_t type;
	uint8_t last_head;
	uint8_t last_cylinder_sector;
	uint8_t last_cylinder;
	uint32_t lba_first;
	uint32_t sectors;
};

packed_struct master_boot_record
{
	uint8_t uarea[440];
	uint32_t signature;
	uint16_t copy_protected;
	struct mbr_partition part0;
	struct mbr_partition part1;
	struct mbr_partition part2;
	struct mbr_partition part3;
	uint16_t boot_signature;
};

/* block_dev interface */

/* Our block_dev implementation redirects calls to parent device, but takes care to add
   the partition offset when handling block address. */

struct mbr_part_data
{
	uint offset;
	uint size;
	struct block_dev *parent;
};

#define mbr_get_part_data(bdev) ((struct mbr_part_data *)(bdev)->opaque)

static uint mbr_part_bd_lock(struct block_dev *dev, uint num)
{
	struct mbr_part_data *part = mbr_get_part_data(dev);

	if (dev->valid == false)
		kpanic("mbr_part_bd_lock(): invalid block device");

	return part->parent->lock(part->parent, num + part->offset);
}

static void mbr_part_bd_unlock(struct block_dev *dev, uint index)
{
	struct mbr_part_data *part = mbr_get_part_data(dev);

	if (dev->valid == false)
		kpanic("mbr_part_bd_unlock(): invalid block device");

	part->parent->unlock(part->parent, index);
}

static void mbr_part_bd_write(struct block_dev *dev, uint index, uint off, const byte *src, uint len)
{
	struct mbr_part_data *part = mbr_get_part_data(dev);

	if (dev->valid == false)
		kpanic("mbr_part_bd_write(): invalid block device");

	part->parent->write(part->parent, index, off, src, len);
}

static void mbr_part_bd_read(struct block_dev *dev, uint index, uint off, byte *dest, uint len)
{
	struct mbr_part_data *part = mbr_get_part_data(dev);

	if (dev->valid == false)
		kpanic("mbr_part_bd_read(): invalid block device");

	part->parent->read(part->parent, index, off, dest, len);
}

static struct block_dev mbr_part_block_dev_template = {
	.name = "MBR part template",
	.valid = false,
	.block_size = 0,
	.num_blocks = 0,
	.opaque = NULL,

	.lock = mbr_part_bd_lock,
	.unlock = mbr_part_bd_unlock,
	.write = mbr_part_bd_write,
	.read = mbr_part_bd_read,
};

/* Partition handling. */

static struct block_dev *mbr_create_partition(struct block_dev *parent, uint part_num, uint offset,
	uint size)
{
	struct block_dev *sub;
	struct mbr_part_data *part;

	sub = kalloc(HEAP_NORMAL, 1, sizeof(struct block_dev));
	part = kalloc(HEAP_NORMAL, 1, sizeof(struct mbr_part_data));

	/* Fill out the block_dev fields. */

	kmemcpy(sub, &mbr_part_block_dev_template, sizeof(struct block_dev));

	sub->valid = true;
	sub->block_size = parent->block_size;
	sub->num_blocks = size;
	sub->opaque = part;

	kmemset(sub->name, 0, sizeof(sub->name));
	ksnprintf(sub->name, sizeof(sub->name) - 1, "%s:%d", parent->name, part_num);

	/* Fill out partition data. */

	part->offset = offset;
	part->size = size;
	part->parent = parent;

	return sub;
}

static bool mbr_handle_partition(struct block_dev *parent, struct mbr_partition *part, uint num)
{
	struct block_dev *sub;

	/* Make sure the partition is actually present. */
	if (part->status != MBR_PART_PRESENT)
		return false;

	/* Make sure offset and length are valid. */
	kassert(part->lba_first != 0 && part->sectors != 0);
	kassert(part->lba_first + part->sectors <= parent->num_blocks);

	/* Create a block_dev representing the partition. */
	sub = mbr_create_partition(parent, num, part->lba_first, part->sectors);

	if (sub == NULL)
		return false;

	/* Register the block device. */
	kdprintf("Found partition %s - MBR part %d type %x, at %u for %u\n",
		sub->name, num, part->type, part->lba_first, part->sectors);

	bdev_add(sub);

	return true;
}

/* kernel/block/partitions.h exports */

bool mbr_table_scan(struct block_dev *bdev)
{
	bool ret = false;
	byte *buf;
	uint idx;
	struct master_boot_record *mbr;

	if (bdev->valid == false)
		return false;

	/* TODO: Is this actually a correct assumption? */
	if (bdev->block_size != MBR_SECTOR_SIZE)
		return false;

	/* Get the first sector from the block device, and see if it contains an MBR partition table. */

	buf = kalloc(HEAP_NORMAL, 1, bdev->block_size);

	idx = bdev->lock(bdev, 0);
	bdev->read(bdev, idx, 0, buf, bdev->block_size);
	bdev->unlock(bdev, idx);

	mbr = (struct master_boot_record *) buf;

	if (mbr->boot_signature == MBR_BOOT_SIGNATURE)
	{
		ret = ret || mbr_handle_partition(bdev, &(mbr->part0), 0);
		ret = ret || mbr_handle_partition(bdev, &(mbr->part1), 1);
		ret = ret || mbr_handle_partition(bdev, &(mbr->part2), 2);
		ret = ret || mbr_handle_partition(bdev, &(mbr->part3), 3);
	}

	kfree(buf);

	return ret;
}
