/* libc/arch/i386/syscall.h - x86 syscall macros */
#ifndef ARCH_I386_SYSCALL_H_
#define ARCH_I386_SYSCALL_H_

/* TODO: Synchronize with kernel somehow */
#define SYSCALL_EXIT 1
#define SYSCALL_BRK 2
#define SYSCALL_SBRK 3

#define asm_syscall "int $0x80"

/* TODO: Which types do we use? */

static inline int syscall0(int num)
{
	int result;

	asm volatile (
		"movl %1, %%eax\n"
		asm_syscall"\n"
		"movl %%eax, %0"
		: "=m" (result)
		: "r" ((num))
		: "eax"
		);

	return result;
}

static inline int syscall1(int num, int arg1)
{
	int result;

	asm volatile (
		"movl %1, %%eax\n"
		"movl %2, %%ebx\n"
		asm_syscall"\n"
		"movl %%eax, %0"
		: "=m" (result)
		: "r" ((num)), "r" ((arg1))
		: "eax", "ebx"
		);

	return result;
}

#endif
