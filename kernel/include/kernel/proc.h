/* proc.h - arch-independent process structure */
#ifndef _KERNEL_PROC_H
#define _KERNEL_PROC_H

#define PID_KERNEL 0

#define PROC_NEW			0 /* Process has just been created. */
#define PROC_READY			2 /* Process is ready to be scheduled. */
#define PROC_DEFUNCT		3 /* Process has exited and is waiting to be collected. */
#define PROC_TRUNCATE		4 /* Process can be deleted. */

struct proc
{
	unsigned int pid;
	int state;
	char name[32];
};

#endif
