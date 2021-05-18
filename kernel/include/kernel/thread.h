/* thread.h - arch-independent threading structure */
#ifndef _KERNEL_THREAD_H
#define _KERNEL_THREAD_H

#include <kernel/ticks.h>

#define KERNEL_TID_IDLE 0

#define THREAD_NEW			0 /* Thread has just been created. */
#define THREAD_RUNNING		1 /* Thread is currently running. */
#define THREAD_SCHEDULER	2 /* Thread is the scheduler thread. */
#define THREAD_READY		3 /* Thread is ready to be scheduled. */
#define THREAD_BLOCKED		4 /* Thread is waiting on a lock. */
#define THREAD_SLEEPING		5 /* Thread is sleeping. */
#define THREAD_EXITED		6 /* Thread has exited. */

struct thread
{
	unsigned int tid;
	int state;
	ticks_t sleep_since;
	ticks_t sleep_until;
	char name[32];
};

#endif
