#include <kernel/heap.h>
#include <kernel/thread.h>
#include <kernel/utils.h>
#include <kernel/vfs.h>
#include <kernel/fs/devfs.h>

#include "devfs_internal.h"

struct thread_mutex devfs_mutex;

struct vfs_super devfs = {
		.bdev = NULL,
		.opaque = NULL,
		.get_root = devfs_get_root,
		.get_by_index = devfs_get_by_index,
		.get_by_path = devfs_get_by_path,
		.put = devfs_put,
};

struct vfs_node devfs_root = {
		.type = VFS_NODE_DIRECTORY,
		.flags = VFS_NODE_BIT_READABLE,
		.index = 1,
		.parent = &devfs,
		.opaque = NULL,

		.lock = devfs_root_lock,
		.unlock = devfs_root_unlock,
		.get_name = devfs_root_get_name,
		.get_size = devfs_root_get_size,
		.read = devfs_root_read,
		.write = devfs_root_write,
		.get_num_leaves = devfs_root_get_num_leaves,
		.get_leaf = devfs_root_get_leaf,
		.get_leaf_node = devfs_root_get_leaf_node,
};

uint devfs_num_leaves;
inode_t devfs_inode_seq = 2;
struct vfs_node_list devfs_nodes;

/* Initialize the devfs registry and super node. */
void devfs_init(void)
{
	thread_mutex_create(&devfs_mutex);
	LIST_INIT(&devfs_nodes);
}

/* Registers a devfs node. */
devfs_handle_t devfs_register_node(struct devfs_node *node, uint vfs_flags)
{
	struct devfs_vfs_node_data *vfs_node_data;
	struct vfs_node *vfs_node;
	devfs_handle_t handle;

	thread_mutex_acquire(&devfs_mutex);

	/* TODO: Check if it exists! */

	/* Create the vfs_node data. */
	vfs_node_data = kualloc(HEAP_NORMAL, sizeof(struct devfs_vfs_node_data));
	thread_mutex_create(&(vfs_node_data->mutex));
	vfs_node_data->ref = 0;
	vfs_node_data->node = node;

	/* Create the vfs_node itself. */
	vfs_node = kualloc(HEAP_NORMAL, sizeof(struct vfs_node));
	vfs_node->type = VFS_NODE_DEVICE;
	vfs_node->flags = vfs_flags;
	vfs_node->index = devfs_inode_seq++;
	vfs_node->parent = &devfs;
	vfs_node->opaque = vfs_node_data;

	vfs_node->lock = devfs_node_lock;
	vfs_node->unlock = devfs_node_unlock;
	vfs_node->get_name = devfs_node_get_name;
	vfs_node->get_size = devfs_node_get_size;
	vfs_node->read = devfs_node_read;
	vfs_node->write = devfs_node_write;
	vfs_node->get_num_leaves = devfs_node_get_num_leaves;
	vfs_node->get_leaf = devfs_node_get_leaf;
	vfs_node->get_leaf_node = devfs_node_get_leaf_node;

	devfs_num_leaves++;

	handle = vfs_node->index;

	LIST_INSERT_HEAD(&devfs_nodes, vfs_node, lptrs);

	thread_mutex_release(&devfs_mutex);

	return handle;
}

/* Unregisters a devfs node. */
void devfs_unregister_node(devfs_handle_t handle)
{
	struct vfs_node *node;
	struct devfs_vfs_node_data *node_data;

	thread_mutex_acquire(&devfs_mutex);

	/* Find the node and panic if it isn't found. TODO: Don't panic! */
	node = devfs_get_by_index(&devfs, handle);

	if (!node)
		kpanic("devfs_unregister_node(): node not found");

	/* Check if node has references. */
	node_data = devfs_get_node_data(node);

	if (node_data->ref > 0)
		kpanic("devfs_unregister_node(): node busy");

	/* Just free everything. */
	kfree(node_data);
	LIST_REMOVE(node, lptrs);
	kfree(node);

	devfs_num_leaves--;

	thread_mutex_release(&devfs_mutex);
}

/* Get the devfs super node. */
struct vfs_super *devfs_get_super(void)
{
	return &devfs;
}
