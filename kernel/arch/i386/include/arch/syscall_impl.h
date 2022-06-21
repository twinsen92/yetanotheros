/* arch/syscall_impl.h - syscall subsystem implementation  */
#ifndef ARCH_I386_SYSCALL_IMPL_H
#define ARCH_I386_SYSCALL_IMPL_H

#include <kernel/cdefs.h>
#include <arch/interrupts.h>
#include <user/yaos2/kernel/types.h>

/*
 * Check if the given value will cause an overflow. We compare to INT_MAX because we transfer
 * errors as negative values.
 */
#define syscall_check_overflow(x) ((x) > INT_MAX)

pid_t syscall_fork(struct isr_frame *frame);

#endif
