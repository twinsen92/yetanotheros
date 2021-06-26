/* kernel/test/fat_test.c - FAT test stuff */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/proc.h>
#include <kernel/scheduler.h>
#include <kernel/utils.h>
#include <kernel/vfs.h>
#include <arch/kernel/cpu.h>

static void dump_tree(struct vfs_super *super, struct vfs_node *parent, struct vfs_node *dir, uint level)
{
	uint num_leaves, i, sp;
	struct vfs_node *leaf;

	kassert(dir->type == VFS_NODE_DIRECTORY);

	num_leaves = dir->get_num_leaves(dir);

	for (i = 0; i < num_leaves; i++)
	{
		leaf = dir->get_leaf_node(dir, i);

		/* Ignore current or parent directory. */
		if (leaf == dir || leaf == parent)
			continue;

		/* Ignore directories pointing to root directory. This might be the case with .. in first
		   level directories. */
		if (leaf->type == VFS_NODE_DIRECTORY && leaf->index == 0)
			continue;

		kassert(leaf);

		for (sp = 0; sp < level; sp++)
		{
			kdprintf(" ");
		}

		leaf->lock(leaf);

		kdprintf("%s\n", leaf->get_name(leaf));

		if (leaf->type == VFS_NODE_DIRECTORY)
		{
			dump_tree(super, dir, leaf, level + 1);
		}

		leaf->unlock(leaf);

		super->put(super, leaf);
	}
}

static void dump_tree_main(struct vfs_super *test)
{
	struct vfs_node *root_node = test->get_root(test);
	root_node->lock(root_node);
	kdprintf("(root)\n");
	dump_tree(test, NULL, root_node, 1);
	root_node->unlock(root_node);
	test->put(test, root_node);
}

static void competing_thread_main(void *cookie)
{
	const char *name = (const char*)cookie;
	uint num;
	char buf[16];

	while (1)
	{
		struct file *f = vfs_open("/something/testA.txt");
		struct file *f2 = vfs_open("/soemthing/testA.txt");

		kassert(f);
		kassert(f2 == NULL);

		while (1)
		{
			num = f->read(f, buf, 1);

			if (num == 0)
				break;

			buf[1] = 0;

			kdprintf("%s%s ", name, buf);

			thread_sleep(200);
		}

		vfs_close(f);
	}
}

noreturn fat_test_main(struct vfs_super *test)
{
	thread_create(PID_KERNEL, competing_thread_main, "A", "FAT test A");
	thread_create(PID_KERNEL, competing_thread_main, "B", "FAT test B");
	thread_create(PID_KERNEL, competing_thread_main, "C", "FAT test C");
	thread_create(PID_KERNEL, competing_thread_main, "D", "FAT test D");
	thread_create(PID_KERNEL, competing_thread_main, "E", "FAT test E");

	dump_tree_main(test);

	while (1)
		cpu_relax();
}
