/* thread_switch.S - x86 thread switch implementation */
.text
.global x86_thread_switch
.func x86_thread_switch
x86_thread_switch:
	# Save caller's register state.
	#
	# Note that the SVR4 ABI allows us to destroy %eax, %ecx, %edx,
	# but requires us to preserve %ebx, %ebp, %esi, %edi.  See
	# [SysV-ABI-386] pages 3-11 and 3-12 for details.
	#
	# This stack frame must match the one set up by thread_create()
	# in size.
	pushl	%ebx
	pushl	%ebp
	pushl	%esi
	pushl	%edi

	# Get offsetof (struct thread, stack).
.global struct_x86_thread_offsetof_esp
	mov struct_x86_thread_offsetof_esp, %edx

	# Save current stack pointer to old thread's stack, if any.
	movl 20(%esp), %eax
	movl %esp, (%eax,%edx,1)

	# Restore stack pointer from new thread's stack.
	movl 24(%esp), %ecx
	movl (%ecx,%edx,1), %esp

	# Restore caller's register state.
	popl %edi
	popl %esi
	popl %ebp
	popl %ebx
	ret
.endfunc
