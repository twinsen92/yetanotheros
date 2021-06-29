/* kernel/syscall.h - syscall subsystem */
#ifndef _KERNEL_SYSCALL_H
#define _KERNEL_SYSCALL_H

#include <kernel/cdefs.h>

void init_syscall(void);

noreturn syscall_exit(void);

#endif
