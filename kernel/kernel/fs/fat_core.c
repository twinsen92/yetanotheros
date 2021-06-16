/* kernel/fs/fat_core.c - base FAT filesystem implementation */
#include <kernel/block.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/endian.h>
#include <kernel/heap.h>
#include <kernel/utils.h>
#include <kernel/vfs.h>
#include <kernel/wchar.h>
#include <kernel/fs/fat_types.h>
#include <kernel/fs/fat_vfs.h>

/* Read the next cluster out of FAT, using a cluster number. Returns FAT_OK, FAT_LAST_CLUSTER,
   FAT_BAD_CLUSTER or FAT_ERROR. */
uint fat_read_fat(struct vfs_super *super, uint32_t *next_cluster, uint32_t cluster)
{
	struct fat_vfs_super_data *fat_data;

	uint32_t fat_offset, fat_sector, ent_offset;
	uint32_t fat_entry;
	uint block_index;

	/* Get the super node data. */
	kassert(super);
	fat_data = (struct fat_vfs_super_data *) super->opaque;

	switch (fat_data->type)
	{
	case FAT12:
		fat_offset = cluster + (cluster / 2);
		break;
	case FAT16:
		fat_offset = cluster * 2;
		break;
	case FAT32:
		fat_offset = cluster * 4;
		break;
	default:
		return FAT_ERROR;
	}

	fat_sector = fat_data->first_fat_sector + (fat_offset / fat_data->bs.bytes_per_sector);
	ent_offset = fat_offset % fat_data->bs.bytes_per_sector;

	block_index = super->bdev->lock(super->bdev, fat_sector);
	super->bdev->read(super->bdev, block_index, ent_offset, (byte*)&fat_entry, sizeof(fat_entry));
	super->bdev->unlock(super->bdev, block_index);

	switch (fat_data->type)
	{
	case FAT12:
		*next_cluster = little_endian_uint16(*((uint16_t*)&fat_entry));

		/* TODO: Figure out what this is (FAT12). */
		if (cluster & 0x0001)
			*next_cluster = *next_cluster >> 4;
		else
			*next_cluster = *next_cluster & 0x0FFF;

		if (*next_cluster >= 0x0ff8)
			return FAT_LAST_CLUSTER;
		else if (*next_cluster == 0x0ff7)
			return FAT_BAD_CLUSTER;

		break;
	case FAT16:
		*next_cluster = little_endian_uint16(*((uint16_t*)&fat_entry));

		if (*next_cluster >= 0xfff8)
			return FAT_LAST_CLUSTER;
		else if (*next_cluster == 0xfff7)
			return FAT_BAD_CLUSTER;

		break;
	case FAT32:
		/* 4 highest bits are reserved */
		*next_cluster = little_endian_uint32(*((uint32_t*)&fat_entry)) & 0x0FFFFFFF;

		if (*next_cluster >= 0x0ffffff8)
			return FAT_LAST_CLUSTER;
		else if (*next_cluster == 0x0ffffff7)
			return FAT_BAD_CLUSTER;

		break;

	case ExFAT:
		kpanic("fat_read_fat(): ExFAT is unsupported");
	}

	return FAT_OK;
}

/* TODO: Implement some sort of caching mechanism. */

/* Walks the path and returns the first cluster of the file or directory pointed at by the path.
   Returns 0 on fault or if the node could not be found. Also writes the FAT entry into result and
   the long file name into the name_buffer (which has to be at least FAT_LFN_NAME_SIZE large). */
uint32_t fat_walk_path(struct fat_entry *result, struct vfs_super *super, const char *path,
	uint32_t root_cluster, char *name_buffer)
{
	const char *node = NULL;
	const char *next = NULL;
	bool is_leaf = false;
	uint node_name_length;
	uint32_t leaf_cluster = 0;
	int en_i = 0;

	/* Get the super node data. */
	kassert(super);

	/* Path has to start with a separator. Will not accept non-sanitized input. */
	kassert(path[0] == VFS_SEPARATOR);
	node = path + 1;

	/* Need to have a name buffer... */
	kassert(name_buffer);

	/* Get the next node in the path. */
	next = kstrchr(node, VFS_SEPARATOR);

	/* If there is no next node, this is leaf (i.e. file). */
	is_leaf = (next == NULL);

	/* Node name's length. */
	if (is_leaf)
		node_name_length = kstrlen(node);
	else
		node_name_length = next - node;

	if (node_name_length >= FAT_LFN_NAME_SIZE)
		kpanic("fat_walk_path(): node name is too long");

	while (leaf_cluster == 0)
	{
		/* Read a FAT entry including LFN entries. */
		en_i = fat_read_entry(result, super, root_cluster, en_i, name_buffer);

		/* We got an error */
		if (en_i == FAT_ENTRY_ERROR)
			kpanic("fat_walk_path(): error reading entry");

		/* Check if this is the node we're looking for. */
		if (kstrncmp(name_buffer, node, node_name_length))
		{
			if (is_leaf && (result->attributes & FAT_DIRECTORY) == 0)
			{
				/* Leaf and the node isn't a directory. This is the leaf we were looking for. */
				leaf_cluster = fat_get_entry_cluster(result);
			}
			else if (!is_leaf && (result->attributes & FAT_DIRECTORY) != 0)
			{
				/* Not leaf and the node is a directory. Start walking from the node. */
				leaf_cluster = fat_walk_path(result, super, next, fat_get_entry_cluster(result),
					name_buffer);
			}
		}

		/* That was the last entry. */
		if (en_i == FAT_ENTRY_LAST)
			break;
	}

	return leaf_cluster;
}

/* Reads bytes from the disk. Returns the number of read bytes. */
int fat_read(struct vfs_super *super, uint32_t first_cluster, void *buf, uint off,
	int num)
{
	int num_read = 0;
	struct fat_vfs_super_data *fat_data;
	uint32_t initial_offset;
	uint32_t cl_i, cl, next_cl, cl_first_sector, cl_portion;
	uint state;
	uint32_t bl_i, bl_portion, block_index;

	if (num <= 0)
		return num_read;

	kassert(super && buf);

	fat_data = fat_get_super_data(super);

	/* We want to know which cluster in sequence the current offset is in. */
	cl_i = off / fat_data->bytes_per_cluster;
	initial_offset = off % fat_data->bytes_per_cluster;
	cl = first_cluster;

	/* Follow the chain to the cluster we want to read. */
	while (cl_i > 0)
	{
		state = fat_read_fat(super, &next_cl, cl);

		if (state != FAT_OK)
			/* TODO: Maybe some more info? */
			return 0;

		cl = next_cl;
		cl_i--;
	}

	/* Read the data from the cluster and read more clusters if needed. */
	while (num > 0)
	{
		cl_first_sector = fat_first_sector_of_cluster(fat_data, cl);

		/* Calculate how much we will read from this cluster. */
		cl_portion = num;

		if (num + initial_offset > fat_data->bytes_per_cluster)
			cl_portion = fat_data->bytes_per_cluster - initial_offset;

		for (bl_i = 0; bl_i <= cl_portion / fat_data->bs.sectors_per_cluster; bl_i++)
		{
			/* Calculate how much we will read from this block. */
			bl_portion = num;

			if (num + initial_offset > fat_data->bs.bytes_per_sector)
				bl_portion = fat_data->bs.bytes_per_sector - initial_offset;

			/* Perform the read. */
			block_index = super->bdev->lock(super->bdev, cl_first_sector + bl_i);
			super->bdev->read(super->bdev, block_index, initial_offset, buf + num_read, bl_portion);
			super->bdev->unlock(super->bdev, block_index);

			/* Update num_read. */
			num_read += bl_portion;
			num -= bl_portion;

			/* We've applied the offset already. Make it 0. */
			initial_offset = 0;
		}

		/* Check if we're done, so that we don't have to read the next fat entry. */
		if (num == 0)
			break;

		/* Read the next cluster in the chain. */
		state = fat_read_fat(super, &next_cl, cl);

		if (state != FAT_OK)
			/* TODO: Maybe some more info? */
			return 0;

		cl = next_cl;
	}

	return num_read;
}

/* Reads an entry from a directory, starting from entry idx. Returns the next entry that can be read
   from the directory, FAT_ENTRY_LAST or FAT_ENTRY_ERROR. Also writes the FAT entry into result and
   the long file name into the name_buffer (which has to be at least FAT_LFN_NAME_SIZE large). */
int fat_read_entry(struct fat_entry *result, struct vfs_super *super, uint32_t first_cluster,
	int idx, char *name_buffer)
{
	byte entry_buffer[FAT_ENTRY_SIZE];
	struct fat_entry *entry;
	struct fat_lfn *lfn;
	int iseq = 0;
	bool in_lfn_sequence = false;

	if (idx < 0)
		return FAT_ENTRY_ERROR;

	/* entry and lfn pointers won't change */
	entry = (struct fat_entry *)entry_buffer;
	lfn = (struct fat_lfn *)entry_buffer;

	while (1)
	{
		/* Read the next entry from disk. */
		if (!fat_read(super, first_cluster, entry_buffer, idx * FAT_ENTRY_SIZE, FAT_ENTRY_SIZE))
			return FAT_ENTRY_ERROR;

		idx++;

		/* Check the first byte of the entry. */
		if (*(entry_buffer) == FAT_LAST_ENTRY)
			return FAT_ENTRY_LAST;
		else if (*(entry_buffer) == FAT_UNUSED_ENTRY)
			continue;

		if (name_buffer && fat_is_lfn_entry(entry_buffer) && !fat_is_lfn_deleted(lfn))
		{
			/* This is an existing LFN entry. */

			if (!in_lfn_sequence)
			{
				/* We're entering an LFN sequence. */
				in_lfn_sequence = true;
				iseq = fat_get_lfn_nr(lfn) - 1;

				kmemset(name_buffer, 0, sizeof(FAT_LFN_NAME_SIZE));
			}
			else
			{
				iseq--;
			}

			kassert(iseq >= 0);

			/* Copy the portion of characters to the entry name. */
			fat_copy_all_ascii_chars(name_buffer + iseq * FAT_LFN_CHARS, lfn);
		}
		else if (!fat_is_lfn_entry(entry_buffer))
		{
			/* This is a normal entry. */

			if (name_buffer && !in_lfn_sequence)
			{
				/* TODO: This is wrong. 20h is used instead of null as empty characters. */
				/* Copy the entry's name (first 8 are file name, 3 last are extension) */

				kmemset(name_buffer, 0, 13);
				kstrncpy(name_buffer, entry->file_name, 8);

				/* Extension might not apply to directories. */
				if (*(entry->file_name + 8) != 0)
				{
					kstrcat(name_buffer, ".");
					kstrncpy(name_buffer + kstrlen(name_buffer), entry->file_name + 8, 3);
				}
			}

			/* End of LFN chain (if any). */
			in_lfn_sequence = false;

			kmemcpy(result, entry, sizeof(struct fat_entry));

			/* We've found an actual entry. Return. */
			break;
		}
	}

	return idx;
}
