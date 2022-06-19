/* kernel/syscall_impl.h - syscall subsystem implementation  */
#ifndef _KERNEL_SYSCALL_IMPL_H
#define _KERNEL_SYSCALL_IMPL_H

#include <kernel/addr.h>
#include <kernel/cdefs.h>

#include <user/yaos2/kernel/types.h>

void init_syscall(void);

noreturn syscall_exit(int status);
pid_t syscall_wait(uvaddr_t status);

int syscall_brk(uvaddr_t ptr);
uvaddr_t syscall_sbrk(uvaddrdiff_t diff);

int syscall_open(uvaddr_t path, int flags);
int syscall_close(int fd);
ssize_t syscall_read(int fd, uvaddr_t buf, size_t count);
ssize_t syscall_write(int fd, uvaddr_t buf, size_t count);

#endif
