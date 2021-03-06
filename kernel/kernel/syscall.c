/* kernel/syscall.c - syscall subsystem */
#include <kernel/cdefs.h>
#include <kernel/syscall.h>
#include <kernel/thread.h>

noreturn syscall_exit(void)
{
	/* TODO: This actually means exit PROCESS. */
	thread_exit();
}
