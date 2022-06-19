/* kernel/vfs/file.c - VFS file interface */
#include <kernel/cdefs.h>
#include <kernel/heap.h>
#include <kernel/thread.h>
#include <kernel/vfs.h>

#include "file.h"

static struct thread_mutex vfs_file_list_mutex;

static struct file_list vfs_file_list;

/* Init file susbsystem. */
void vfs_file_init(void)
{
	thread_mutex_create(&vfs_file_list_mutex);
	LIST_INIT(&vfs_file_list);
}

/* Opens a new file object using the path. Returns null if not found. */
struct file *vfs_open(const char *path)
{
	struct vfs_node *node;
	struct file *file;

	node = vfs_get(path);

	if (node == NULL)
		return NULL;

	thread_mutex_acquire(&vfs_file_list_mutex);

	/* Create a new file object. */
	file = kualloc(HEAP_NORMAL, sizeof(struct file));

	/* Fill out the data. */
	file->node = node;
	file->opaque = NULL;
	file->ref = 1;
	file->offset = 0;

	/* Set up the interface. */
	file->read = vfs_file_read;
	file->write = vfs_file_write;
	file->seek_beg = vfs_file_seek_beg;

	/* Register it */
	LIST_INSERT_HEAD(&vfs_file_list, file, lptrs);

	thread_mutex_release(&vfs_file_list_mutex);

	return file;
}

struct file *vfs_file_dup(struct file *f)
{
	if (f == NULL)
		kpanic("vfs_file_dup(): attempt to dup NULL file");

	/* Increase reference counter. That's it. */
	f->node->lock(f->node);
	f->ref++;
	f->node->unlock(f->node);

	return f;
}

/* Closes an open file object. */
void vfs_close(struct file *f)
{
	bool last_ref = false;

	if (f == NULL)
		kpanic("vfs_close(): attempt to close NULL file");

	/* Decrease reference counter. */
	f->node->lock(f->node);
	f->ref--;

	if (f->ref == 0)
		last_ref = true;

	f->node->unlock(f->node);

	/* Get rid of the file object and the underlying VFS node if no one else is using it. */
	if (last_ref)
	{
		thread_mutex_acquire(&vfs_file_list_mutex);
		LIST_REMOVE(f, lptrs);
		thread_mutex_release(&vfs_file_list_mutex);

		vfs_put(f->node);
		kfree(f);
	}
}
