#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/thread.h>
#include <kernel/vfs.h>

int vfs_file_read(struct file *f, void *buf, int num)
{
	int result = 0;

	if (num <= 0)
		return result;

	/* TODO: We should have something like this that does sanity checks for all "methods" */
	kassert(f->node->lock != NULL);

	f->node->lock(f->node);

	if (f->offset >= f->node->get_size(f->node))
		goto read_eof;

	result = f->node->read(f->node, buf, f->offset, num);
	f->offset += result;

read_eof:
	f->node->unlock(f->node);

	return result;
}

int vfs_file_write(struct file *f, const void *buf, int num)
{
	int result = 0;

	if (num <= 0)
		return result;

	kassert(f->node->lock != NULL);

	f->node->lock(f->node);
	result = f->node->write(f->node, buf, f->offset, num);
	f->offset += result;
	f->node->unlock(f->node);

	return result;
}

void vfs_file_seek_beg(struct file *f, uint offset)
{
	kassert(f->node->lock != NULL);

	f->node->lock(f->node);
	f->offset = offset;
	f->node->unlock(f->node);
}
