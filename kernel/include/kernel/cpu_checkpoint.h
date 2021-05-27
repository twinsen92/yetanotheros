/* cpu_checkpoint.h - spin checkpoint for CPUs */
#ifndef _KERNEL_CPU_CHECKPOINT_H
#define _KERNEL_CPU_CHECKPOINT_H

#include <kernel/cdefs.h>

struct cpu_checkpoint
{
	atomic_uint num;
};

void cpu_checkpoint_create(struct cpu_checkpoint *cp);
void cpu_checkpoint_enter(struct cpu_checkpoint *cp);

#endif
