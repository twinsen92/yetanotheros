/* kernel/syscall_impl.h - syscall subsystem implementation  */
#ifndef _KERNEL_SYSCALL_IMPL_H
#define _KERNEL_SYSCALL_IMPL_H

#include <kernel/cdefs.h>

void init_syscall(void);

noreturn syscall_exit(int status);

#endif
