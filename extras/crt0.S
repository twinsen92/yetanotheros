.data
.global _argv0
_argv0:
	.ascii "crt0\0"

.global _argv
_argv:
	.long _argv0

.text
.global _start
_start:
	movl %esp, %ebp

	call _yaos2_libc_init

	# TODO: Pass something useful to the main procedure.
	# argv
	movl _argv, %eax
	pushl %eax
	# argc
	pushl $1
	# main(argc, argv)
	call main

	call _yaos2_libc_atexit_entry

	# syscall1 (SYSCALL_EXIT, %eax)
	movl %eax, %ebx
	movl $1, %eax
	int $0x80

1:
	jmp 1b
