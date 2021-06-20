/* kernel/fs/fat_vfs.c - VFS interface to the FAT driver */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/heap.h>
#include <kernel/queue.h>
#include <kernel/thread.h>
#include <kernel/utils.h>
#include <kernel/vfs.h>
#include <kernel/fs/fat_types.h>
#include <kernel/fs/fat_vfs.h>

/* vfs_super interface */

static bool unsafe_truncate(struct vfs_super *super)
{
	struct fat_vfs_super_data *fat_data;
	struct vfs_node *node;
	struct fat_vfs_node_data *node_data;
	uint lowest_hit = UINT_MAX;
	struct vfs_node *lowest_node = NULL;

	fat_data = fat_get_super_data(super);

	/* Do not truncate if we're below threshold. */
	if (fat_data->nof_nodes <= FAT_VFS_NODE_TRUNCATE_THRESHOLD)
		return false;

	/* Look for an unreferenced node with lowest hit rate. */
	LIST_FOREACH(node, &(fat_data->node_list), lptrs)
	{
		node_data = fat_get_node_data(node);

		if (node_data->ref == 0 && node_data->hit < lowest_hit)
		{
			lowest_hit = node_data->hit;
			lowest_node = node;
		}
	}

	/* If we found a node, remove it. */
	if (lowest_node)
	{
		LIST_REMOVE(lowest_node, lptrs);
		fat_data->nof_nodes--;
		kfree(lowest_node->opaque);
		kfree(lowest_node);
		return true;
	}

	return false;
}

static struct vfs_node *unsafe_fat_get(struct vfs_super *super, uint32_t first_cluster)
{
	struct fat_vfs_super_data *fat_data;
	struct vfs_node *node;
	struct fat_vfs_node_data *node_data;

	fat_data = fat_get_super_data(super);

	/* First, look for an existing node with the same first_cluster. */
	LIST_FOREACH(node, &(fat_data->node_list), lptrs)
	{
		node_data = fat_get_node_data(node);

		if (node_data->first_cluster == first_cluster)
			return node;
	}

	return NULL;
}

static struct vfs_node *unsafe_create(struct vfs_super *super, struct fat_result *result)
{
	struct vfs_node *node;
	struct fat_vfs_node_data *node_data;

	node = kalloc(HEAP_NORMAL, 1, sizeof(struct vfs_node));
	node_data = kalloc(HEAP_NORMAL, 1, sizeof(struct fat_vfs_node_data));

	kmemset(node, 0, sizeof(struct vfs_node));
	kmemset(node_data, 0, sizeof(struct fat_vfs_node_data));

	/* TODO: Support more flags. */
	node->type = VFS_NODE_FILE;
	node->flags = VFS_NODE_BIT_READABLE;

	if (result->entry.attributes & FAT_DIRECTORY)
		node->type = VFS_NODE_DIRECTORY;

	node->index = fat_get_entry_cluster(&(result->entry));
	node->parent = super;
	node->opaque = node_data;

	node_data->dir_cluster = result->dir_cluster;
	node_data->first_cluster = fat_get_entry_cluster(&(result->entry));
	node_data->ref = 0;
	node_data->hit = 0;
	thread_mutex_create(&(node_data->mutex));
	node_data->num_bytes = result->entry.num_bytes;

	node->lock = fat_vfs_node_lock;
	node->unlock = fat_vfs_node_unlock;
	node->get_size = fat_vfs_node_get_size;
	node->read = fat_vfs_node_read;
	node->write = fat_vfs_node_write;
	node->get_num_leaves = fat_vfs_node_get_num_leaves;
	node->get_leaf = fat_vfs_node_get_leaf;
	node->get_leaf_node = fat_vfs_node_get_leaf_node;

	return node;
}

static struct vfs_node *safe_get_with_fat_result(struct vfs_super *super, struct fat_result *result)
{
	struct fat_vfs_super_data *fat_data;
	struct vfs_node *node;
	struct fat_vfs_node_data *node_data;

	fat_data = fat_get_super_data(super);

	thread_mutex_acquire(&(fat_data->mutex));
	node = unsafe_fat_get(super, fat_get_entry_cluster(&(result->entry)));

	if (node == NULL)
	{
		node = unsafe_create(super, result);

		fat_data->nof_nodes++;
		LIST_INSERT_HEAD(&(fat_data->node_list), node, lptrs);
	}

	node_data = fat_get_node_data(node);
	node_data->ref++;
	node_data->hit++;
	thread_mutex_release(&(fat_data->mutex));

	return node;
}

/* vfs_super interface */

/* Get the root node. */
struct vfs_node *fat_vfs_get_root(struct vfs_super *super)
{
	struct fat_result result;

	/* Fill out a fake result. */
	result.dir_cluster = FAT_INVALID_CLUSTER;
	kmemcpy(&(result.entry), &(fat_get_super_data(super)->root_entry), sizeof(struct fat_entry));
	kmemset(result.lfn, 0, sizeof(result.lfn));

	return safe_get_with_fat_result(super, &result);
}

/* Get a node by its index. */
struct vfs_node *fat_vfs_get_by_index(__unused struct vfs_super *super, __unused inode_t index)
{
	kpanic("fat_vfs_get_by_index(): not implemented");
}

/* Get a node by its path. */
struct vfs_node *fat_vfs_get_by_path(struct vfs_super *super, const char *path)
{
	struct fat_result result;
	uint32_t cluster;

	cluster = fat_get_entry_cluster(&(fat_get_super_data(super)->root_entry));

	if (fat_walk_path(&result, super, path, cluster) != FAT_OK)
		return NULL;

	return safe_get_with_fat_result(super, &result);
}

/* Return a node to be collected by the super node. */
void fat_vfs_put(struct vfs_super *super, struct vfs_node *node)
{
	struct fat_vfs_super_data *fat_data;
	struct fat_vfs_node_data *node_data;

	fat_data = fat_get_super_data(super);
	node_data = fat_get_node_data(node);

	thread_mutex_acquire(&(fat_data->mutex));
	node_data->ref--;

	/* Possibly truncate a node. */
	unsafe_truncate(super);

	thread_mutex_release(&(fat_data->mutex));
}

/* vfs_node interface */

/* Locks the node to prevent other threads from modifying it. */
void fat_vfs_node_lock(struct vfs_node *node)
{
	thread_mutex_acquire(&(fat_get_node_data(node)->mutex));
}

/* Unlocks the node to allowe other threads to modify it. */
void fat_vfs_node_unlock(struct vfs_node *node)
{
	thread_mutex_release(&(fat_get_node_data(node)->mutex));
}

/* Get the size of this node. */
uint fat_vfs_node_get_size(struct vfs_node *node)
{
	struct fat_vfs_node_data *node_data;

	node_data = fat_get_node_data(node);

	if (!thread_mutex_held(&(node_data->mutex)))
		kpanic("fat_vfs_node_get_size(): mutex not held");

	return node_data->num_bytes;
}

/* Read num bytes starting at off into buf. Put the actual number of bytes read into p_num_read. */
int fat_vfs_node_read(struct vfs_node *node, void *buf, uint off, int num)
{
	struct fat_vfs_node_data *node_data;

	if (num <= 0)
		return 0;

	node_data = fat_get_node_data(node);

	if (!thread_mutex_held(&(node_data->mutex)))
		kpanic("fat_vfs_node_read(): mutex not held");

	/* Check if we're at the end of the file. */
	if (node_data->num_bytes <= off)
		return 0;

	/* Make sure we don't read too much. */
	if (node_data->num_bytes < off + num)
		num = node_data->num_bytes - off;

	return fat_read(node->parent, node_data->first_cluster, buf, off, num);
}

/* Write num bytes starting at off, using the contents of buf. This replaces existing bytes and
   possibly expands the file. Returns the number of written bytes. */
int fat_vfs_node_write(__unused struct vfs_node *node, __unused const void *buf, __unused uint off,
	__unused int num)
{
	if (num <= 0)
		return 0;

	kpanic("fat_vfs_node_write(): unimplemented");
}

/* If this node is a directory, get the number of leaves in it. For other types of nodes, this
   is always 1. */
uint fat_vfs_node_get_num_leaves(struct vfs_node *node)
{
	struct fat_vfs_node_data *node_data;

	node_data = fat_get_node_data(node);

	if (!thread_mutex_held(&(node_data->mutex)))
		kpanic("fat_vfs_node_get_num_leaves(): mutex not held");

	if (node->type != VFS_NODE_DIRECTORY)
		return 1;

	kpanic("fat_vfs_node_get_num_leaves(): not implemented");
}

/* Get leaf number n. First "0" leaf always points at itself. */
inode_t fat_vfs_node_get_leaf(struct vfs_node *node, uint n)
{
	return fat_vfs_node_get_leaf_node(node, n)->index;
}

struct vfs_node *fat_vfs_node_get_leaf_node(struct vfs_node *node, uint n)
{
	struct fat_vfs_node_data *node_data;

	node_data = fat_get_node_data(node);

	if (!thread_mutex_held(&(node_data->mutex)))
		kpanic("fat_vfs_node_get_leaf_node(): mutex not held");

	if (n == 0)
		return node;

	if (node->type != VFS_NODE_DIRECTORY)
		kpanic("fat_vfs_node_get_leaf_node(): n > 0 for non-directory node");

	kpanic("fat_vfs_node_get_leaf_node(): not implemented");
}
