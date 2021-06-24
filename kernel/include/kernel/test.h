/* test.h  - kernel mode test procedures */
#ifndef _KERNEL_TEST_H
#define _KERNEL_TEST_H

#include <kernel/vfs.h>

noreturn kalloc_test_main(void);

noreturn fat_test_main(struct vfs_super *test);

#endif
