
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/utils.h>
#include <kernel/vfs.h>

#include "devfs_internal.h"

/* vfs_super interface */

struct vfs_node *devfs_get_root(__unused struct vfs_super *super)
{
	return &devfs_root;
}

struct vfs_node *devfs_get_by_index(__unused struct vfs_super *super, inode_t index)
{
	struct vfs_node *node;
	struct devfs_vfs_node_data *node_data;

	thread_mutex_acquire(&devfs_mutex);

	LIST_FOREACH(node, &devfs_nodes, lptrs)
		if (node->index == index)
			break;

	if (node)
	{
		node_data = devfs_get_node_data(node);
		node_data->ref++;
	}

	thread_mutex_release(&devfs_mutex);

	return node;
}

struct vfs_node *devfs_get_by_path(__unused struct vfs_super *super, const char *path)
{
	const char *name;
	struct vfs_node *node;
	struct devfs_vfs_node_data *node_data;

	/* Make sure the path is properly formatted. We expect it to look like this: /file1 */
	if (path[0] != VFS_SEPARATOR)
		kpanic("devfs_get_by_path(): incorrect path format");
	if (path[kstrlen(path)-1] == VFS_SEPARATOR)
		kpanic("devfs_get_by_path(): incorrect path format");

	name = path + 1;

	thread_mutex_acquire(&devfs_mutex);

	LIST_FOREACH(node, &devfs_nodes, lptrs)
	{
		node_data = devfs_get_node_data(node);

		if (kstrcmp(name, node_data->node->name))
			break;
	}

	if (node)
	{
		node_data = devfs_get_node_data(node);
		node_data->ref++;
	}

	thread_mutex_release(&devfs_mutex);

	return node;
}

void devfs_put(__unused struct vfs_super *super, struct vfs_node *node)
{
	struct devfs_vfs_node_data *node_data;

	thread_mutex_acquire(&devfs_mutex);
	node_data = devfs_get_node_data(node);
	node_data->ref--;
	thread_mutex_release(&devfs_mutex);
}

/* root vfs_node interface */

void devfs_root_lock(__unused struct vfs_node *node)
{
	thread_mutex_acquire(&devfs_mutex);
}

void devfs_root_unlock(__unused struct vfs_node *node)
{
	thread_mutex_release(&devfs_mutex);
}

const char *devfs_root_get_name(__unused struct vfs_node *node)
{
	return "";
}

uint devfs_root_get_size(__unused struct vfs_node *node)
{
	return 0;
}

int devfs_root_read(__unused struct vfs_node *node, __unused void *buf, __unused uint off, __unused int num)
{
	return 0;
}

int devfs_root_write(__unused struct vfs_node *node, __unused const void *buf, __unused uint off, __unused int num)
{
	return 0;
}

uint devfs_root_get_num_leaves(__unused struct vfs_node *node)
{
	kassert(thread_mutex_held(&devfs_mutex));
	return devfs_num_leaves;
}

inode_t devfs_root_get_leaf(struct vfs_node *node, uint n)
{
	struct vfs_node *leaf_node;
	inode_t inode = 0;

	if (n == 0)
		return node->index;

	thread_mutex_acquire(&devfs_mutex);

	LIST_FOREACH(leaf_node, &devfs_nodes, lptrs)
		if (--n == 0)
			break;

	if (leaf_node)
		inode = leaf_node->index;

	thread_mutex_release(&devfs_mutex);

	return inode;
}

struct vfs_node *devfs_root_get_leaf_node(struct vfs_node *node, uint n)
{
	struct vfs_node *leaf_node;
	struct devfs_vfs_node_data *leaf_node_data;

	if (n == 0)
		return node;

	thread_mutex_acquire(&devfs_mutex);

	LIST_FOREACH(leaf_node, &devfs_nodes, lptrs)
		if (--n == 0)
			break;

	if (leaf_node)
	{
		leaf_node_data = devfs_get_node_data(leaf_node);
		leaf_node_data->ref++;
	}

	thread_mutex_release(&devfs_mutex);

	return leaf_node;
}
