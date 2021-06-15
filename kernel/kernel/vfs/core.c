/* kernel/vfs/core.c - core VFS functionality */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/vfs.h>

static struct vfs_super *vfs_root = NULL;

/* Initializes the virtual filesystem. The super node provided in argument will be available at /.*/
void vfs_init(struct vfs_super *root)
{
	vfs_root = root;
}

/* Get a vfs_node object using the path. Returns null if it could not be created. */
struct vfs_node *vfs_get(const char *path)
{
	if (vfs_root == NULL)
		kpanic("vfs_get(): no root filesystem");

	return vfs_root->get_by_path(vfs_root, path);
}

/* Put (decrement reference) a vfs_node back into the virtual filesystem. */
void vfs_put(struct vfs_node *node)
{
	if (vfs_root == NULL)
		kpanic("vfs_put(): no root filesystem");

	return vfs_root->put(vfs_root, node);
}
