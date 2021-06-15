/* kernel/fs/fat.h - VFS interface to the FAT filesystem driver */
#ifndef _KERNEL_VFS_FAT_H
#define _KERNEL_VFS_FAT_H

#include <kernel/block.h>
#include <kernel/vfs.h>

struct vfs_super *fat_create_super(struct block_dev *bdev);

#endif
