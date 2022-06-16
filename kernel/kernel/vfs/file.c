/* kernel/vfs/file.c - VFS file interface */
#include <kernel/cdefs.h>
#include <kernel/heap.h>
#include <kernel/thread.h>
#include <kernel/vfs.h>

static int file_read(struct file *f, void *buf, int num)
{
	int result = 0;

	if (num <= 0)
		return result;

	/* TODO: Is this mutex really necessary? */
	thread_mutex_acquire(&(f->mutex));
	kassert(f->node->lock != NULL);
	f->node->lock(f->node);

	if (f->offset >= f->node->get_size(f->node))
		goto read_eof;

	result = f->node->read(f->node, buf, f->offset, num);
	f->offset += result;

read_eof:
	f->node->unlock(f->node);
	thread_mutex_release(&(f->mutex));

	return result;
}

static int file_write(struct file *f, const void *buf, int num)
{
	int result = 0;

	if (num <= 0)
		return result;

	thread_mutex_acquire(&(f->mutex));
	kassert(f->node->lock != NULL);
	f->node->lock(f->node);
	result = f->node->write(f->node, buf, f->offset, num);
	f->offset += result;
	f->node->unlock(f->node);
	thread_mutex_release(&(f->mutex));

	return result;
}

static void file_seek_beg(struct file *f, uint offset)
{
	thread_mutex_acquire(&(f->mutex));
	f->offset = offset;
	thread_mutex_release(&(f->mutex));
}

/* Opens a new file object using the path. Returns null if not found. */
struct file *vfs_open(const char *path)
{
	struct vfs_node *node;
	struct file *file;

	node = vfs_get(path);

	if (node == NULL)
		return NULL;

	/* Create a new file object. */
	file = kalloc(HEAP_NORMAL, 1, sizeof(struct file));

	/* Fill out the data. */
	file->node = node;
	thread_mutex_create(&(file->mutex));
	file->offset = 0;
	file->opaque = NULL;

	/* Set up the interface. */
	file->read = file_read;
	file->write = file_write;
	file->seek_beg = file_seek_beg;

	return file;
}

/* Closes an open file object. */
void vfs_close(struct file *f)
{
	if (f == NULL)
		kpanic("vfs_close(): attempt to close NULL file");

	kfree(f);
}
