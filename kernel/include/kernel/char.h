/* kernel/char.h - char device interface */

#include <kernel/cdefs.h>

typedef int cdev_handle_t;

#define CDEV_INVALID_HANDLE 0

/* Owned by registering code. */
struct char_dev
{
	char name[80];
	void *opaque;

	size_t (*read)(struct char_dev *cdev, void *buf, size_t count);
	size_t (*write)(struct char_dev *cdev, const void *buf, size_t count);
};

void cdev_init(void);
cdev_handle_t cdev_register(struct char_dev *cdev, uint vfs_flags);
void cdev_unregister(cdev_handle_t handle);
