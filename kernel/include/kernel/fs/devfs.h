/* kernel/fs/devfs.h - devfs filesystem declarations and constants */
#ifndef _KERNEL_FS_DEVFS_H
#define _KERNEL_FS_DEVFS_H

#include <kernel/cdefs.h>
#include <kernel/vfs.h>

typedef inode_t devfs_handle_t;
#define DEVFS_INVALID_HANDLE 0

/* Initialize the devfs registry and super node. */
void devfs_init(void);

/* Owned by registering code. */
struct devfs_node
{
	/* Constant part. */

	char name[80];
	void *opaque;

	/* Interface. */

	/* Get the size of this node. */
	foffset_t (*get_size)(struct devfs_node *node);

	/* Read num bytes starting at off into buf. Returns the number of read bytes. */
	int (*read)(struct devfs_node *node, void *buf, uint off, int num);

	/* Write num bytes starting at off into buf. Returns the number of written bytes. */
	int (*write)(struct devfs_node *node, const void *buf, uint off, int num);
};

/* Registers a devfs node. */
devfs_handle_t devfs_register_node(struct devfs_node *node, uint vfs_flags);

/* Unregisters a devfs node. */
void devfs_unregister_node(devfs_handle_t handle);

/* Get the devfs super node. */
struct vfs_super *devfs_get_super(void);

#endif
