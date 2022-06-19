/* libc/unistd.c - unistd.h implementation */

#include <yaos2/kernel/syscalls.h>

#include <yaos2/arch/syscall.h>

#include <sys/types.h>
#include <errno.h>
#include <stddef.h>

static inline int set_errno_and_convert(int ret)
{
	if (ret < 0)
	{
		errno = -ret;
		return -1;
	}

	return ret;
}

int brk(void *ptr)
{
	int ret = syscall1(SYSCALL_BRK, (int)ptr);
	return set_errno_and_convert(ret);
}

void *sbrk(int increment)
{
	int ret = syscall1(SYSCALL_SBRK, increment);
	return (void*)set_errno_and_convert(ret);
}

int open(const char *path, int flags)
{
	int ret = syscall2(SYSCALL_OPEN, (int)path, flags);
	return set_errno_and_convert(ret);
}

int close(int fd)
{
	int ret = syscall1(SYSCALL_CLOSE, fd);
	return set_errno_and_convert(ret);
}

ssize_t read(int fd, void *buf, size_t count)
{
	int ret = syscall3(SYSCALL_READ, fd, (int)buf, count);
	return set_errno_and_convert(ret);
}

ssize_t write(int fd, const void *buf, size_t count)
{
	int ret = syscall3(SYSCALL_WRITE, fd, (int)buf, count);
	return set_errno_and_convert(ret);
}

