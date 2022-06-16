#ifndef _KERNEL_FS_DEVFS_INTERNAL_H
#define _KERNEL_FS_DEVFS_INTERNAL_H

#include <kernel/cdefs.h>
#include <kernel/thread.h>
#include <kernel/vfs.h>
#include <kernel/fs/devfs.h>

extern struct thread_mutex devfs_mutex;
extern struct vfs_super devfs;
extern struct vfs_node devfs_root;
extern uint devfs_num_leaves;
extern inode_t devfs_inode_seq;
extern struct vfs_node_list devfs_nodes;

struct vfs_node *devfs_get_root(struct vfs_super *super);
struct vfs_node *devfs_get_by_index(struct vfs_super *super, inode_t index);
struct vfs_node *devfs_get_by_path(struct vfs_super *super, const char *path);
void devfs_put(struct vfs_super *super, struct vfs_node *node);

void devfs_root_lock(struct vfs_node *node);
void devfs_root_unlock(struct vfs_node *node);
const char *devfs_root_get_name(struct vfs_node *node);
uint devfs_root_get_size(struct vfs_node *node);
int devfs_root_read(struct vfs_node *node, void *buf, uint off, int num);
int devfs_root_write(struct vfs_node *node, const void *buf, uint off, int num);
uint devfs_root_get_num_leaves(struct vfs_node *node);
inode_t devfs_root_get_leaf(struct vfs_node *node, uint n);
struct vfs_node *devfs_root_get_leaf_node(struct vfs_node *node, uint n);

struct devfs_vfs_node_data
{
	/* Super part. To modify, super lock has to be held.*/

	uint ref;

	/* Dynamic part. (protected by node mutex) */

	struct thread_mutex mutex;
	struct devfs_node *node;
};

#define devfs_get_node_data(node) ((struct devfs_vfs_node_data *)(node)->opaque)

void devfs_node_lock(struct vfs_node *node);
void devfs_node_unlock(struct vfs_node *node);
const char *devfs_node_get_name(struct vfs_node *node);
uint devfs_node_get_size(struct vfs_node *node);
int devfs_node_read(struct vfs_node *node, void *buf, uint off, int num);
int devfs_node_write(struct vfs_node *node, const void *buf, uint off, int num);
uint devfs_node_get_num_leaves(struct vfs_node *node);
inode_t devfs_node_get_leaf(struct vfs_node *node, uint n);
struct vfs_node *devfs_node_get_leaf_node(struct vfs_node *node, uint n);

#endif
