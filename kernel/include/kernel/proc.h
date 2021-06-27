/* kernel/proc.h - arch-independent process structure */
#ifndef _KERNEL_PROC_H
#define _KERNEL_PROC_H

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

#endif
