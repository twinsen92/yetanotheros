/* kernel/fs/fat_types.h - FAT filesystem declarations and constants */
#ifndef _KERNEL_FS_FAT_VFS_H
#define _KERNEL_FS_FAT_VFS_H

#include <kernel/cdefs.h>
#include <kernel/thread.h>
#include <kernel/vfs.h>
#include <kernel/fs/fat_types.h>

/* kernel/vfs.h interface related */

/* Minimum number of VFS nodes in VFS super until nodes will start being truncated. */
#define FAT_VFS_TRUNCATE_THRESHOLD 30

/* Maximum number of leaves that can be stored in the VFS node cache. */
#define FAT_VFS_CACHE_LEAVES 16

struct fat_vfs_super_data
{
	/* Constant part. */

	struct fat_bootsec bs;

	fat_type_t type;

	uint32_t num_sectors;
	uint32_t fat_size;
	uint32_t num_root_dir_sectors;
	uint32_t first_data_sector;
	uint32_t first_fat_sector;
	uint32_t num_data_sectors;
	uint32_t num_clusters;
	uint32_t first_root_dir_sector;
	uint32_t entries_per_cluster;
	uint32_t bytes_per_cluster;

	struct fat_entry root_entry;

	/* Dynamic part. */
	struct thread_mutex mutex;
	uint nof_nodes;
	struct vfs_node_list node_list;
};

static inline uint32_t fat_first_sector_of_cluster(const struct fat_vfs_super_data *s,
	uint32_t cluster)
{
	return (cluster - FAT_CLUSTER_OFFSET) * s->bs.sectors_per_cluster + s->first_data_sector;
}

#define FAT_VFS_LEAVES_DIRTY 0x01 /* Leaves have changed dirty flag. */
#define FAT_VFS_ENTRY_DIRTY 0x02 /* Entry details have changed dirty flag. */

struct fat_vfs_node_data
{
	/* Super part. To modify, super lock has to be held.*/

	uint32_t dir_cluster; /* Directory cluster where the node entry is located. */
	uint32_t first_cluster; /* First cluster on disk that contains the data of the node. */
	uint ref; /* Current number of references to this node. */
	uint hit; /* Number of times this node has been found. */

	/* Dynamic part. (protected by node mutex) */

	struct thread_mutex mutex;
	uint flags; /* When set, node has been dirtied (modified somehow). */

	/* Cache part. (protected by node mutex) */
	char name[FAT_LFN_NAME_SIZE]; /* File name. */
	uint32_t num_bytes; /* Number of bytes the node occupies. */
	uint num_leaves; /* Number of leaves stored in leaves array. */
	int leaves[FAT_VFS_CACHE_LEAVES]; /* Indices where leaves start. */
};

#define fat_get_super_data(super) ((struct fat_vfs_super_data *)(super)->opaque)
#define fat_get_node_data(node) ((struct fat_vfs_node_data *)(node)->opaque)

/* vfs_super interface */

/* Get the root node. */
struct vfs_node *fat_vfs_get_by_index(struct vfs_super *super, inode_t index);

/* Get a node by its index. */
struct vfs_node *fat_vfs_get_root(struct vfs_super *super);

/* Get a node by its path. */
struct vfs_node *fat_vfs_get_by_path(struct vfs_super *super, const char *path);

/* Return a node to be collected by the super node. */
void fat_vfs_put(struct vfs_super *super, struct vfs_node *node);

/* vfs_node interface */

/* Locks the node to prevent other threads from modifying it. */
void fat_vfs_node_lock(struct vfs_node *node);

/* Unlocks the node to allowe other threads to modify it. */
void fat_vfs_node_unlock(struct vfs_node *node);

/* Get the name of this node. */
const char *fat_vfs_node_get_name(struct vfs_node *node);

/* Get the size of this node. */
uint fat_vfs_node_get_size(struct vfs_node *node);

/* Read num bytes starting at off into buf. Returns the number of read bytes. */
int fat_vfs_node_read(struct vfs_node *node, void *buf, uint off, int num);

/* Write num bytes starting at off, using the contents of buf. This replaces existing bytes and
   possibly expands the file. Returns the number of written bytes. */
int fat_vfs_node_write(struct vfs_node *node, const void *buf, uint off, int num);

/* If this node is a directory, get the number of leaves in it. For other types of nodes, this
   is always 1. */
uint fat_vfs_node_get_num_leaves(struct vfs_node *node);

/* Get leaf number n. First "0" leaf always points at itself. */
inode_t fat_vfs_node_get_leaf(struct vfs_node *node, uint n);

/* Get leaf number n. First "0" leaf always points at itself. */
struct vfs_node *fat_vfs_node_get_leaf_node(struct vfs_node *node, uint n);

#endif
