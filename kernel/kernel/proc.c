/* kernel/proc.c - arch-independent process susbsystem interface */
#include <kernel/debug.h>
#include <kernel/proc.h>
#include <kernel/thread.h>
#include <kernel/vfs.h>

#include <user/yaos2/kernel/errno.h>

void proc_lock(struct proc *proc)
{
	thread_mutex_acquire(&(proc->mutex));
}

void proc_unlock(struct proc *proc)
{
	thread_mutex_release(&(proc->mutex));
}

int proc_get_vacant_fd(struct proc *proc)
{
	int fd;

	kassert(thread_mutex_held(&(proc->mutex)));

	for (fd = 0; fd < PROC_MAX_FILES; fd++)
		if (proc->opened_files[fd] == NULL)
			return fd;

	return -EUNSPEC;
}

struct file *proc_get_file(struct proc *proc, int fd)
{
	kassert(thread_mutex_held(&(proc->mutex)));

	if (fd < 0 || fd >= PROC_MAX_FILES)
		return NULL;

	return proc->opened_files[fd];
}

int proc_bind(struct proc *proc, int fd, struct file *file)
{
	kassert(thread_mutex_held(&(proc->mutex)));

	if (fd < 0 || fd >= PROC_MAX_FILES)
		return -EUNSPEC;

	if (proc->opened_files[fd] != NULL)
		return -EUNSPEC;

	proc->opened_files[fd] = file;

	return fd;
}

struct file *proc_unbind(struct proc *proc, int fd)
{
	struct file *f;

	kassert(thread_mutex_held(&(proc->mutex)));

	if (fd < 0 || fd >= PROC_MAX_FILES)
		return NULL;

	f = proc->opened_files[fd];
	proc->opened_files[fd] = NULL;

	return f;
}
