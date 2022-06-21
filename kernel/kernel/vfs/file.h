/* kernel/vfs/file.h - internal vfs file subsystem declarations */
#ifndef _KERNEL_VFS_FILE_H
#define _KERNEL_VFS_FILE_H

#include <kernel/cdefs.h>
#include <kernel/vfs.h>

void vfs_file_init(void);

int vfs_file_read(struct file *f, void *buf, int num);
int vfs_file_write(struct file *f, const void *buf, int num);
foffset_t vfs_file_seek(struct file *f, foffset_t offset, int whence);

#endif
