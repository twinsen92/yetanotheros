/* user/yaos2/arch/syscall.h - x86 syscall macros (user-space API definitions) */
#ifndef _USER_YAOS2_I386_SYSCALL_H_
#define _USER_YAOS2_I386_SYSCALL_H_

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
		: "m" ((num))
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
		: "m" ((num)), "m" ((arg1))
		: "eax", "ebx"
		);

	return result;
}

static inline int syscall2(int num, int arg1, int arg2)
{
	int result;

	asm volatile (
		"movl %1, %%eax\n"
		"movl %2, %%ebx\n"
		"movl %3, %%ecx\n"
		asm_syscall"\n"
		"movl %%eax, %0"
		: "=m" (result)
		: "m" ((num)), "m" ((arg1)), "m" ((arg2))
		: "eax", "ebx", "ecx"
		);

	return result;
}

static inline int syscall3(int num, int arg1, int arg2, int arg3)
{
	int result;

	asm volatile (
		"movl %1, %%eax\n"
		"movl %2, %%ebx\n"
		"movl %3, %%ecx\n"
		"movl %4, %%edx\n"
		asm_syscall"\n"
		"movl %%eax, %0"
		: "=m" (result)
		: "m" ((num)), "m" ((arg1)), "m" ((arg2)), "m" ((arg3))
		: "eax", "ebx", "ecx", "edx"
		);

	return result;
}

#endif
