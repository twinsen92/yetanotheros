/* user/yaos2/kernel/syscalls.h - syscall subsystem function numbers (user-space API definitions) */
#ifndef _USER_YAOS2_KERNEL_SYSCALLS_H
#define _USER_YAOS2_KERNEL_SYSCALLS_H

enum syscall_num
{
	SYSCALL_EXIT = 1,
	SYSCALL_WAIT,
	SYSCALL_FORK,

	SYSCALL_BRK,
	SYSCALL_SBRK,

	SYSCALL_OPEN,
	SYSCALL_CLOSE,
	SYSCALL_READ,
	SYSCALL_WRITE,
};

#endif
