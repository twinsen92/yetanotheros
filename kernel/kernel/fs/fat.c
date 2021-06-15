/* kernel/fs/fat.c - kernel FAT driver */
#include <kernel/block.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/endian.h>
#include <kernel/heap.h>
#include <kernel/queue.h>
#include <kernel/thread.h>
#include <kernel/utils.h>
#include <kernel/vfs.h>
#include <kernel/fs/fat_types.h>
#include <kernel/fs/fat_vfs.h>

/* kernel/fs/fat.h interface */

#define fat_check_label(pext) (kstrncmp((pext)->fat_type_label, "FAT", 3))

struct vfs_super *fat_create_super(struct block_dev *bdev)
{
	struct vfs_super *super = NULL;
	struct fat_vfs_super_data *data = NULL;
	struct fat_bootsec *bs = NULL;
	uint block_index;
	uint32_t num_sectors = 0;
	uint32_t fat_size = 0;
	uint32_t num_root_dir_sectors = 0;
	uint32_t num_data_sectors = 0;
	uint32_t num_clusters = 0;
	fat_type_t type;

	/* NOTE Maybe convert the header? Decide if we support big endian architectures. */
	kassert(CURRENT_ENDIANNESS == LITTLE_ENDIAN);

	bs = kalloc(HEAP_NORMAL, 1, sizeof(struct fat_bootsec));

	/* Read the volume's boot sector. */
	block_index = bdev->lock(bdev, 0);
	bdev->read(bdev, block_index, 0, (byte*)bs, sizeof(struct fat_bootsec));
	bdev->unlock(bdev, block_index);

	/* Check the FAT type label. */
	if (!fat_check_label(&(bs->ext16)) && !fat_check_label(&(bs->ext32)))
		goto failed;

	/* Make sure the sector size is equal to our own. */
	if (bs->bytes_per_sector != bdev->block_size)
		goto failed;

	/* Get some information about the filesystem from the boot sector. */
	/* Total number of sectors of the volume. */
	if (bs->total_sectors_16 != 0)
		num_sectors = bs->total_sectors_16;
	else
		num_sectors = bs->total_sectors_32;

	/* Size of the FAT. */
	if (bs->table_size_16 != 0)
		fat_size = bs->table_size_16;
	else
		fat_size = bs->ext32.table_size_32;

	/* Number of sectors occupied by root directory (FAT12/16). */
	num_root_dir_sectors = (bs->root_entry_count * FAT_ENTRY_SIZE + bs->bytes_per_sector - 1)
			/ bs->bytes_per_sector;

	/* Number of data sectors. */
	num_data_sectors = num_sectors
			- (bs->reserved_sector_count + bs->table_count * fat_size + num_root_dir_sectors);

	/* Number of clusters. */
	num_clusters = num_data_sectors / bs->sectors_per_cluster;

	/* TODO: Support more than FAT32. */
	if(num_clusters < 4085)
	{
		type = FAT12;
		goto failed;
	}
	else if(num_clusters < 65525)
	{
		type = FAT16;
		goto failed;
	}
	else if (num_clusters < 268435445)
	{
		type = FAT32;
	}
	else
	{
		type = ExFAT;
		goto failed;
	}

	/* We know enough to create a new filesystem object. */
	super = kalloc(HEAP_NORMAL, 1, sizeof(struct vfs_super));
	data = kalloc(HEAP_NORMAL, 1, sizeof(struct fat_vfs_super_data));

	kmemset(super, 0, sizeof(struct vfs_super));
	kmemset(data, 0, sizeof(struct fat_vfs_super_data));

	super->bdev = bdev;
	super->opaque = data;

	/* Fill out interface. */
	super->get_root = fat_vfs_get_root;
	super->get_by_index = fat_vfs_get_by_index;
	super->get_by_path = fat_vfs_get_by_path;
	super->put = fat_vfs_put;

	kmemcpy(&(data->bs), bs, sizeof(struct fat_bootsec));

	data->type = type;
	data->num_sectors = num_sectors;
	data->fat_size = fat_size;
	data->num_root_dir_sectors = num_root_dir_sectors;
	data->num_data_sectors = num_data_sectors;
	data->num_clusters = num_clusters;

	/* First sector in which directories and files are stored. */
	data->first_data_sector = bs->reserved_sector_count + bs->table_count * fat_size
			+ num_root_dir_sectors;

	/* First sector of the FAT. */
	data->first_fat_sector = bs->reserved_sector_count;

	if(num_clusters < 4085 || num_clusters < 65525)
		/* FAT12/16 */
		data->first_root_dir_sector = data->first_data_sector - num_root_dir_sectors;
	else if (num_clusters < 268435445)
		/* FAT32 */
		data->first_root_dir_sector = fat_first_sector_of_cluster(data, bs->ext32.root_cluster);

	/* TODO: This is just for our convenience. Maybe this shouldn't be a constraint? */
	kassert(((bs->sectors_per_cluster * bs->bytes_per_sector) % FAT_ENTRY_SIZE) == 0);

	data->bytes_per_cluster = bs->sectors_per_cluster * bs->bytes_per_sector;
	data->entries_per_cluster = data->bytes_per_cluster / FAT_ENTRY_SIZE;

	/* TODO: Are we doing the right thing? */
	data->root_entry.attributes = FAT_SYSTEM | FAT_DIRECTORY;
	data->root_entry.cluster_high = (bs->ext32.root_cluster >> 16) & 0xffff;
	data->root_entry.cluster_low = bs->ext32.root_cluster & 0xffff;

	/* Create the node cache. */
	thread_mutex_create(&(data->mutex));
	data->nof_nodes = 0;
	LIST_INIT(&(data->node_list));

	return super;

failed:
	if (bs)
		kfree(bs);

	return NULL;
}
