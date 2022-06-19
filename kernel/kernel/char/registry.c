#include <kernel/char.h>
#include <kernel/cpu.h>
#include <kernel/heap.h>
#include <kernel/queue.h>
#include <kernel/utils.h>
#include <kernel/fs/devfs.h>

struct char_dev_backend
{
	cdev_handle_t handle;
	struct char_dev *cdev;

	devfs_handle_t devfs_handle;
	struct devfs_node devfs_node;

	LIST_ENTRY(char_dev_backend) lptrs;
};

LIST_HEAD(char_dev_backend_list, char_dev_backend);

static struct cpu_spinlock cdev_spinlock;
static cdev_handle_t cdev_next_handle = 1;
static struct char_dev_backend_list cdev_registry;

static uint cdev_devfs_get_size(struct devfs_node *node);
static int cdev_devfs_read(struct devfs_node *node, void *buf, uint off, int num);
static int cdev_devfs_write(struct devfs_node *node, const void *buf, uint off, int num);

void cdev_init(void)
{
	cpu_spinlock_create(&cdev_spinlock, "character device registry spinlock");
	LIST_INIT(&cdev_registry);
}

cdev_handle_t cdev_register(struct char_dev *cdev, uint vfs_flags)
{
	struct char_dev_backend *backend;
	cdev_handle_t handle = CDEV_INVALID_HANDLE;

	cpu_spinlock_acquire(&cdev_spinlock);

	LIST_FOREACH(backend, &cdev_registry, lptrs)
		if (kstrcmp(backend->cdev->name, cdev->name))
			kpanic("cdev_register(): char device exists");

	backend = kzualloc(HEAP_NORMAL, sizeof(struct char_dev_backend));
	handle = cdev_next_handle++;
	backend->handle = handle;
	backend->cdev = cdev;
	kstrncpy(backend->devfs_node.name, cdev->name, sizeof(backend->devfs_node.name) - 1);
	backend->devfs_node.opaque = cdev;
	backend->devfs_node.get_size = cdev_devfs_get_size;
	backend->devfs_node.read = cdev_devfs_read;
	backend->devfs_node.write = cdev_devfs_write;
	backend->devfs_handle = devfs_register_node(&(backend->devfs_node), vfs_flags);

	LIST_INSERT_HEAD(&cdev_registry, backend, lptrs);

	cpu_spinlock_release(&cdev_spinlock);

	return handle;
}

void cdev_unregister(__unused cdev_handle_t handle)
{
	kpanic("cdev_unregister(): not implemented");
}

/* devfs interface */

#define get_cdev(node) ((struct char_dev*)node->opaque)

static uint cdev_devfs_get_size(__unused struct devfs_node *node)
{
	return 0;
}

static int cdev_devfs_read(struct devfs_node *node, void *buf, __unused uint off, int num)
{
	return get_cdev(node)->read(get_cdev(node), buf, num);
}

static int cdev_devfs_write(struct devfs_node *node, const void *buf, __unused uint off, int num)
{
	return get_cdev(node)->write(get_cdev(node), buf, num);
}
