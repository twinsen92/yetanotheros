#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/thread.h>
#include <kernel/vfs.h>

#include <user/yaos2/kernel/defs.h>

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

foffset_t vfs_file_seek(struct file *f, foffset_t offset, int whence)
{
	foffset_t cur;

	kassert(f->node->lock != NULL);

	f->node->lock(f->node);

	if (whence == SEEK_SET)
		f->offset = offset;
	else if (whence == SEEK_CUR)
		f->offset += offset;
	else if (whence == SEEK_END)
		f->offset = f->node->get_size(f->node) + offset;

	cur = f->offset;

	f->node->unlock(f->node);

	return cur;
}
