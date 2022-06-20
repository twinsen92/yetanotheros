/* kernel/proc.h - arch-independent process structure */
#ifndef _KERNEL_PROC_H
#define _KERNEL_PROC_H

#include <user/yaos2/kernel/types.h>
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/queue.h>
#include <kernel/thread.h>
#include <kernel/vfs.h>

#define PID_KERNEL 0

enum proc_state
{
	/* Process has just been created. */
	PROC_NEW = 0,

	/* Process is ready to be scheduled. */
	PROC_READY,

	/*
	 * Process has started exiting due to exit(). We're waiting until all threads exit before we
	 * can mark it as DEFUNCT.
	 */
	PROC_EXITING,

	/* Process has exited and is waiting to be collected. */
	PROC_DEFUNCT,

	/* Process can be deleted. */
	PROC_TRUNCATE,
};

#define PROC_MAX_FILES 16

struct proc
{
	/* Constant part. */

	pid_t pid;
	pid_t parent;
	char name[32];
	struct arch_proc *arch;

	/* Dynamic part. Protected with global scheduler lock. */

	int state;
	int exit_status;
	struct thread_list threads; /* Thread list. */

	/* Dynamic part. Protected with process mutex. */

	struct thread_mutex mutex;
	struct file *opened_files[PROC_MAX_FILES]; /* TODO: Get rid of this array. */

	LIST_ENTRY(proc) pointers;
};

LIST_HEAD(proc_list, proc);

/* Creates a new proc object. */
struct proc *proc_alloc(const char *name);

/* Releases a given proc object and all related resources. */
void proc_free(struct proc *proc);

#define VM_USER 0x01
#define VM_WRITE 0x02
#define VM_EXEC 0x04

/* Reserves a physical page for the virtual memory page pointed at by v. */
void proc_vmreserve(struct proc *proc, uvaddr_t v, uint flags);

/* Read from the process' virtual memory. */
void proc_vmread(struct proc *proc, uvaddr_t v, void *buf, size_t num);

/* Write to the process' virtual memory. */
void proc_vmwrite(struct proc *proc, uvaddr_t v, const void *buf, size_t num);

/* Set the main thread stack pointer. */
void proc_set_stack(struct proc *proc, uvaddr_t v, size_t size);

/* Set the break pointer. */
void proc_set_break(struct proc *proc, uvaddr_t v);

/* Set process virtual memory. */
void proc_set_uvm(struct proc *proc);

/* Set kernel virtual memory. */
void proc_set_kvm(void);

/* Changes the break pointer. Returns -1 on error. */
int proc_brk(struct proc *proc, uvaddr_t v);

/* Changes the break pointer. Returns -1 on error. */
uvaddr_t proc_sbrk(struct proc *proc, uvaddrdiff_t diff);

void proc_lock(struct proc *proc);
void proc_unlock(struct proc *proc);
int proc_get_vacant_fd(struct proc *proc);
struct file *proc_get_file(struct proc *proc, int fd);
int proc_bind(struct proc *proc, int fd, struct file *file);
struct file *proc_unbind(struct proc *proc, int fd);

noreturn proc_exit(int status);
pid_t proc_wait(int *status);

#endif
