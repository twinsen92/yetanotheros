/* arch/syscall_impl.h - syscall subsystem implementation  */
#ifndef ARCH_I386_SYSCALL_IMPL_H
#define ARCH_I386_SYSCALL_IMPL_H

#include <arch/interrupts.h>

#include <user/yaos2/kernel/types.h>

pid_t syscall_fork(struct isr_frame *frame);

#endif
