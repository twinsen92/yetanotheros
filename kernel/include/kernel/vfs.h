/* kernel/vfs.h - kernel filesystem */
#ifndef _KERNEL_VFS_H
#define _KERNEL_VFS_H

#include <kernel/block.h>
#include <kernel/cdefs.h>
#include <kernel/queue.h>
#include <kernel/thread.h>

#define VFS_SEPARATOR '/'

typedef uint inode_t;

struct vfs_node;

/* A struct that describes a filesystem super node that contains normal filesystem nodes. */
struct vfs_super
{
	struct block_dev *bdev;

	void *opaque;

	/* Get the root node. */
	struct vfs_node * (*get_root)(struct vfs_super *super);

	/* Get a node by its index. */
	struct vfs_node * (*get_by_index)(struct vfs_super *super, inode_t index);

	/* Get a node by its path. */
	struct vfs_node * (*get_by_path)(struct vfs_super *super, const char *path);

	/* Return a node to be collected by the super node. */
	void (*put)(struct vfs_super *super, struct vfs_node *node);
};

/* File types. */
#define VFS_NODE_FILE				1
#define VFS_NODE_DIRECTORY			2
#define VFS_NODE_DEVICE				3

/* File flag bits. */
#define VFS_NODE_BIT_READABLE		0x02
#define VFS_NODE_BIT_WRITEABLE		0x04

/* A filesystem node. */
struct vfs_node
{
	/* Constant part. */

	uint type; /* Type of node. */
	uint flags; /* Node flags. */
	inode_t index; /* Node index. */
	struct vfs_super *parent; /* Parent super node. */

	void *opaque; /* Filesystem-specific information. */

	/* Locks the node to prevent other threads from modifying it. */
	void (*lock)(struct vfs_node *node);

	/* Unlocks the node to allowe other threads to modify it. */
	void (*unlock)(struct vfs_node *node);

	/* Get the size of this node. */
	uint (*get_size)(struct vfs_node *node);

	/* Read num bytes starting at off into buf. Returns the number of read bytes. */
	int (*read)(struct vfs_node *node, void *buf, uint off, int num);

	/* Write num bytes starting at off into buf. This replaces existing bytes and possibly expands
	   the file. Returns the number of written bytes. */
	int (*write)(struct vfs_node *node, const void *buf, uint off, int num);

	/* If this node is a directory, get the number of leaves in it. For other types of nodes, this
	   is always 1. */
	uint (*get_num_leaves)(struct vfs_node *node);

	/* Get leaf number n. First "0" leaf always points at itself. */
	inode_t (*get_leaf)(struct vfs_node *node, uint n);

	/* Get leaf number n. First "0" leaf always points at itself. */
	struct vfs_node * (*get_leaf_node)(struct vfs_node *node, uint n);

	LIST_ENTRY(vfs_node) lptrs;
};

LIST_HEAD(vfs_node_list, vfs_node);

/* Initializes the virtual filesystem. The super node provided in argument will be available at /.*/
void vfs_init(struct vfs_super *root);

/* Get a vfs_node object using the path. Returns null if it could not be created. */
struct vfs_node *vfs_get(const char *path);

/* Put (decrement reference) a vfs_node back into the virtual filesystem. */
void vfs_put(struct vfs_node *node);

/* Filesystem node reference with a read/write marker. */
struct file
{
	/* Constant part. */
	struct vfs_node *node;

	/* Dynamic part, protected by mutex. */
	struct thread_mutex mutex;
	uint offset;

	void *opaque;

	int (*read)(struct file *f, void *buf, int num);
	int (*write)(struct file *f, const void *buf, int num);
	/* TODO: Add seek. */
};

/* Opens a new file object using the path. Returns null if not found. All files are open in
   "overwrite" mode. */
struct file *vfs_open(const char *path);

/* Closes an open file object. */
void vfs_close(struct file *f);

#endif
