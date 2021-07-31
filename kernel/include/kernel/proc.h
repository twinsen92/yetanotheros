/* kernel/proc.h - arch-independent process structure */
#ifndef _KERNEL_PROC_H
#define _KERNEL_PROC_H

#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/queue.h>
#include <kernel/thread.h>

#define PID_KERNEL 0

#define PROC_NEW			0 /* Process has just been created. */
#define PROC_READY			2 /* Process is ready to be scheduled. */
#define PROC_DEFUNCT		3 /* Process has exited and is waiting to be collected. */
#define PROC_TRUNCATE		4 /* Process can be deleted. */

typedef unsigned int pid_t;

struct proc
{
	pid_t pid;
	int state;
	char name[32];

	struct arch_proc *arch;

	struct thread_list threads; /* Thread list. */

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
void proc_vmreserve(struct proc *proc, vaddr_t v, uint flags);

/* Write to the process' virtual memory. */
void proc_vmwrite(struct proc *proc, vaddr_t v, const void *buf, size_t num);

/* Set the break pointer. */
void proc_set_break(struct proc *proc, vaddr_t v);

/* Changes the break pointer. Returns -1 on error. */
int proc_brk(struct proc *proc, vaddr_t v);

/* Changes the break pointer. Returns -1 on error. */
vaddr_t proc_sbrk(struct proc *proc, vaddrdiff_t diff);

#endif
