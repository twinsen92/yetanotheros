/* libc/arch/i386/stdlib.c - x86 stdlib.h implementation */

#include <yaos2/kernel/syscalls.h>
#include <yaos2/arch/syscall.h>

void exit(int status)
{
	syscall1(SYSCALL_EXIT, status);
}
