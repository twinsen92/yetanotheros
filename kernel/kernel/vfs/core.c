/* kernel/vfs/core.c - core VFS functionality */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/heap.h>
#include <kernel/queue.h>
#include <kernel/thread.h>
#include <kernel/utils.h>
#include <kernel/vfs.h>

/* TODO: Yeah, this should be redesigned :) Many issues can be caused by this. */
struct vfs_mount
{
	char *path;
	struct vfs_super *super_node;

	LIST_ENTRY(vfs_mount) lptrs;
};

LIST_HEAD(vfs_mount_list, vfs_mount);

static struct thread_mutex vfs_mount_mutex;
static struct vfs_mount_list vfs_mounts;

/* Initializes the virtual filesystem. The super node provided in argument will be available at /.*/
void vfs_init(void)
{
	thread_mutex_create(&vfs_mount_mutex);
}

void vfs_mount(const char *path, struct vfs_super *super_node)
{
	struct vfs_mount *mount;

	/* TODO: Sanitize path. */

	/* Dirty hack: Due to the way we parse the path in vfs_get, we actually mount the root at "" */
	if (kstrcmp(path, "/"))
		path = "";

	if (path[kstrlen(path) - 1] == VFS_SEPARATOR)
		kpanic("vfs_mount(): path must not end with /");

	thread_mutex_acquire(&vfs_mount_mutex);

	LIST_FOREACH(mount, &vfs_mounts, lptrs)
		if (kstrcmp(path, mount->path))
			break;

	if (mount)
		kpanic("vfs_mount(): mount point occupied");

	mount = kualloc(HEAP_NORMAL, sizeof(struct vfs_mount));
	mount->path = kzualloc(HEAP_NORMAL, kstrlen(path) + 1);
	kstrcpy(mount->path, path);
	mount->super_node = super_node;

	LIST_INSERT_HEAD(&vfs_mounts, mount, lptrs);

	thread_mutex_release(&vfs_mount_mutex);
}

struct vfs_super *vfs_umount(__unused const char *path)
{
	kpanic("vfs_umount(): not implemented");
}

/* Get a vfs_node object using the path. Returns null if it could not be created. */
struct vfs_node *vfs_get(const char *path)
{
	size_t path_len;
	struct vfs_mount *mount;
	size_t mount_path_len, min_len;
	char last_char = 0;
	struct vfs_node *node = NULL;

	path_len = kstrlen(path);

	/* TODO: Replace this with an R/W lock to speed things up! */
	thread_mutex_acquire(&vfs_mount_mutex);

	LIST_FOREACH(mount, &vfs_mounts, lptrs)
	{
		mount_path_len = kstrlen(mount->path);
		min_len = kmin(mount_path_len, path_len);

		/* Make sure the beginning of the requested path matches the beginning of the mount point. */
		if (!kstrncmp(path, mount->path, min_len))
			continue;

		/* Make sure the path looks like this: /mount_point or /mount_point/ */
		last_char = path[min_len];

		if (last_char == 0)
		{
			/* We're looking for the root of the mount point. */
			node = mount->super_node->get_root(mount->super_node);
			break;
		}
		else if (last_char == VFS_SEPARATOR)
		{
			node = mount->super_node->get_by_path(mount->super_node, path + min_len);
			break;
		}
	}

	thread_mutex_release(&vfs_mount_mutex);

	return node;
}

/* Put (decrement reference) a vfs_node back into the virtual filesystem. */
void vfs_put(struct vfs_node *node)
{
	struct vfs_super *parent = node->parent;
	return parent->put(parent, node);
}
