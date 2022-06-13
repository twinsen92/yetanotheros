/* libc/arch/i386/stdlib.c - x86 stdlib.h implementation */

#include "syscall.h"

__attribute__((__noreturn__))
void exit(int status)
{
	syscall1(SYSCALL_EXIT, status);
}
