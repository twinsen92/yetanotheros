
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/thread.h>
#include <kernel/vfs.h>
#include <kernel/fs/devfs.h>

#include "devfs_internal.h"

void devfs_node_lock(struct vfs_node *node)
{
	thread_mutex_acquire(&(devfs_get_node_data(node)->mutex));
}

void devfs_node_unlock(struct vfs_node *node)
{
	thread_mutex_release(&(devfs_get_node_data(node)->mutex));
}

const char *devfs_node_get_name(struct vfs_node *node)
{
	/* TODO: Not sure if the mutex has to be held to do this... */
	return devfs_get_node_data(node)->node->name;
}

foffset_t devfs_node_get_size(struct vfs_node *node)
{
	struct devfs_vfs_node_data *node_data = devfs_get_node_data(node);
	kassert(thread_mutex_held(&(node_data->mutex)));
	return node_data->node->get_size(node_data->node);
}

int devfs_node_read(struct vfs_node *node, void *buf, uint off, int num)
{
	struct devfs_vfs_node_data *node_data = devfs_get_node_data(node);
	kassert(thread_mutex_held(&(node_data->mutex)));
	return node_data->node->read(node_data->node, buf, off, num);
}

int devfs_node_write(struct vfs_node *node, const void *buf, uint off, int num)
{
	struct devfs_vfs_node_data *node_data = devfs_get_node_data(node);
	kassert(thread_mutex_held(&(node_data->mutex)));
	return node_data->node->write(node_data->node, buf, off, num);
}

uint devfs_node_get_num_leaves(__unused struct vfs_node *node)
{
	return 0;
}

inode_t devfs_node_get_leaf(struct vfs_node *node, uint n)
{
	if (n > 0)
		kpanic("devfs_node_get_leaf(): requested leaf n > 0");

	return node->index;
}

struct vfs_node *devfs_node_get_leaf_node(struct vfs_node *node, uint n)
{
	if (n > 0)
		kpanic("devfs_node_get_leaf_node(): requested leaf n > 0");

	return node;
}
