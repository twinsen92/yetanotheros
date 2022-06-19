
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/heap.h>
#include <kernel/proc.h>
#include <kernel/scheduler.h>
#include <kernel/thread.h>
#include <kernel/utils.h>
#include <kernel/vfs.h>

#include <user/yaos2/kernel/errno.h>

#define SYSCALL_MAX_PATH_LEN 511

/* Buffer size for big reads or writes. */
#define SYSCALL_FILE_BUFFER 512

int syscall_open(uvaddr_t path, int flags)
{
	/* TODO: Introduce some flags. */

	struct file *file;
	struct proc *proc;
	int len;
	char *kpath;
	int fd;

	/* We need to know how long this path is... */
	len = kmstrlen((const char*)path, SYSCALL_MAX_PATH_LEN);

	proc_set_kvm();
	proc = get_current_proc();

	/* Copy the path to the kernel's virtual memory. */
	kpath = kualloc(HEAP_NORMAL, len + 1);
	proc_vmread(proc, path, kpath, len);
	kpath[len] = 0;

	/* Try to open the file. */
	file = vfs_open(kpath);

	if (!file)
	{
		fd = -EUNSPEC;
	}
	else
	{
		proc_lock(proc);

		/* Try to allocate an fd for the file. */
		fd = proc_get_vacant_fd(proc);

		if (fd < 0)
			vfs_close(file);
		else
			proc_bind(proc, fd, file);

		proc_unlock(proc);
	}

	kfree(kpath);

	proc_set_uvm(proc);

	return fd;
}

int syscall_close(int fd)
{
	int ret = 0;
	struct proc *proc;
	struct file *file;

	proc_set_kvm();
	proc = get_current_proc();

	proc_lock(proc);
	file = proc_unbind(proc, fd);

	if (file != NULL)
		vfs_close(file);
	else
		ret = -EUNSPEC;

	proc_unlock(proc);

	proc_set_uvm(proc);

	return ret;
}

ssize_t syscall_read(int fd, uvaddr_t buf, size_t count)
{
	void *kbuf;
	struct proc *proc;
	struct file *file;
	size_t batch;
	ssize_t num_read, ret = 0;

	proc_set_kvm();
	proc = get_current_proc();

	/*
	 * Increment the ref counter in the file object. This way we will be sure we don't read from
	 * a closed file, but won't have to hold the process lock.
	 */
	proc_lock(proc);
	file = proc_get_file(proc, fd);

	if (file == NULL)
	{
		proc_unlock(proc);
		proc_set_uvm(proc);
		return -EUNSPEC;
	}

	file = vfs_file_dup(file);
	proc_unlock(proc);

	/* Create a kernel buffer. */
	kbuf = kzualloc(HEAP_NORMAL, SYSCALL_FILE_BUFFER);

	while (count > 0)
	{
		/* Calculate how much we're going to read. */
		if (count / SYSCALL_FILE_BUFFER > 0)
			batch = SYSCALL_FILE_BUFFER;
		else
			batch = count % SYSCALL_FILE_BUFFER;

		/* Read from the file and check result. */
		num_read = file->read(file, kbuf, batch);

		if (num_read < 0)
		{
			ret = -EUNSPEC;
			goto early_exit;
		}
		else if (num_read == 0)
			goto early_exit;

		/* Write to process memory. */
		proc_vmwrite(proc, buf + ret, kbuf, batch);

		ret += num_read;
		count -= num_read;
	}

early_exit:
	/* Decrement the ref counter. We're done with the file. */
	vfs_close(file);

	kfree(kbuf);

	proc_set_uvm(proc);

	return ret;
}

ssize_t syscall_write(int fd, uvaddr_t buf, size_t count)
{
	void *kbuf;
	struct proc *proc;
	struct file *file;
	size_t batch;
	ssize_t num_written = 0, ret = 0;

	proc_set_kvm();
	proc = get_current_proc();

	/*
	 * Increment the ref counter in the file object. This way we will be sure we don't read from
	 * a closed file, but won't have to hold the process lock.
	 */
	proc_lock(proc);
	file = proc_get_file(proc, fd);

	if (file == NULL)
	{
		proc_unlock(proc);
		proc_set_uvm(proc);
		return -EUNSPEC;
	}

	file = vfs_file_dup(file);
	proc_unlock(proc);

	/* Create a kernel buffer. */
	kbuf = kzualloc(HEAP_NORMAL, SYSCALL_FILE_BUFFER);

	while (count > 0)
	{
		/* Calculate how much we're going to read. */
		if (count / SYSCALL_FILE_BUFFER > 0)
			batch = SYSCALL_FILE_BUFFER;
		else
			batch = count % SYSCALL_FILE_BUFFER;

		/* Read from process memory. */
		proc_vmread(proc, buf + num_written, kbuf, batch);

		/* Write to the file and check result. */
		num_written = file->write(file, kbuf, batch);

		if (num_written < 0)
		{
			ret = -EUNSPEC;
			goto early_exit;
		}
		else if (num_written == 0)
			goto early_exit;

		ret += num_written;
		count -= num_written;
	}

early_exit:
	/* Decrement the ref counter. We're done with the file. */
	vfs_close(file);

	kfree(kbuf);

	proc_set_uvm(proc);

	return ret;
}
