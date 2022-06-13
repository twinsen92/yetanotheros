/* libc/arch/i386/unistd.c - x86 unistd.h implementation */

#include "syscall.h"

int brk(void *ptr)
{
	return syscall1(SYSCALL_BRK, (int)ptr);
}

void *sbrk(int increment)
{
	return (void*)syscall1(SYSCALL_SBRK, increment);
}
