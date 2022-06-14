/* libc/unistd.c - unistd.h implementation */

#include <yaos2/kernel/syscalls.h>
#include <yaos2/arch/syscall.h>
#include <errno.h>

int brk(void *ptr)
{
	int ret = syscall1(SYSCALL_BRK, (int)ptr);

	if (ret == -1)
		errno = ENOMEM;

	return ret;
}

void *sbrk(int increment)
{
	int ret = syscall1(SYSCALL_SBRK, increment);

	if (ret == -1)
		errno = ENOMEM;

	return (void *)ret;
}
