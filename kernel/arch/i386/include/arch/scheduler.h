/* scheduler.h - x86 arch-dependent scheduler interface */
#ifndef ARCH_I386_SCHEDULER_H
#define ARCH_I386_SCHEDULER_H

#include <kernel/cdefs.h>
#include <arch/thread.h>

/* Initializes the global scheduler data and locks. */
void init_global_scheduler(void);

/* Enters the scheduler. This creates a scheduler thread in the kernel process for the current
   CPU and starts the scheduler loop. This is final. */
noreturn enter_scheduler(void);

/* Gets the object of the thread currently running on the current CPU. */
struct x86_thread *get_current_thread(void);

#endif
