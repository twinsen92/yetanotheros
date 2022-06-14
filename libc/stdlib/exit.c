/* libc/stdlibc/exit.c - exit/atexit implementation */

#include <yaos2/kernel/syscalls.h>
#include <yaos2/arch/syscall.h>
#include <stddef.h>
#include <stdlib.h>

void _yaos2_libc_atexit_entry(void);

static void (*atexit_func)(void) = NULL;

void exit(int status)
{
	_yaos2_libc_atexit_entry();
	syscall1(SYSCALL_EXIT, status);
}

int atexit(void (*func)(void))
{
	/* TODO: Support more atexit slots. */
	if (atexit_func != NULL)
		return -1;
	atexit_func = func;
	return 0;
}

void _yaos2_libc_atexit_entry(void)
{
	if (atexit_func != NULL)
		atexit_func();
}
