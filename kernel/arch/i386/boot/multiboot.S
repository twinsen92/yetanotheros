/* multiboot.S - x86 boot code. This file contains _start, which is the first procedure the bootloader
   calls. */

#include <arch/memlayout.h>
#include <arch/paging_types.h>

# Declare constants for the multiboot header.
.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set FLAGS,    ALIGN | MEMINFO  # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot

# Declare a header as in the Multiboot Standard.
.section .multiboot
.align 4096
	.long MAGIC
	.long FLAGS
	.long CHECKSUM
	# Reserve room for the rest of the multiboot header.
	.skip 8180

.data
.align 4096

#define KERNEL_PD_SELF_ENTRY (__boot_kernel_pd - ASM_KM_VIRT_BASE + PAGE_RW_PRESENT)
#define ZERO_ADDR_ENTRY (__boot_zero_pt - ASM_KM_VIRT_BASE + PAGE_RW_PRESENT)

__boot_kernel_pd:
	# 0x0 -> 0x0, 4 MiBs, RW, present
	.long ZERO_ADDR_ENTRY
	.fill 767, 4, 0
	# 0xC0000000 -> 0x0, 4 MiBs, RW, present
	.long ZERO_ADDR_ENTRY
	.fill 254, 4, 0
	# Current page directory. Accessed via current_pd and current_page_tables in paging.h
	.long KERNEL_PD_SELF_ENTRY

__boot_zero_pt:
	.fill 1024, 4, 0

.global yaos2_initialized
yaos2_initialized:
	.long 0

# Reserve a stack for the initial thread.
.section .bss
.align 16
.global __boot_stack_top
__boot_stack_top:
	.skip 16384 # 16 KiB
.global __boot_stack_bottom
__boot_stack_bottom:

# The kernel entry point.
.section .text
.global _start
.type _start, @function
_start:
	movl $0x00000003, %ecx
	leal (__boot_zero_pt - ASM_KM_VIRT_BASE), %edx
	movl %ecx, (%edx)
1:
	addl $0x00001000, %ecx
	addl $4, %edx
	movl %ecx, (%edx)
	cmpl $0x003ff003, %ecx
	jne 1b

	# Set the page directory
	leal (__boot_kernel_pd - ASM_KM_VIRT_BASE), %ecx
	movl %ecx, %cr3

	# Set the paging bit
	movl %cr0, %ecx
	orl $0x80000000, %ecx
	movl %ecx, %cr0

	# Set the global page enable bit
	movl %cr4, %ecx
	orl $0x00000080, %ecx
	movl %ecx, %cr4

	leal _start_with_paging, %ecx
	jmp *%ecx

_start_with_paging:
	movl $__boot_stack_bottom, %esp
	movl %esp, %ebp

	# Push 0 for an unwinder.
	pushl $0

	# Multiboot magic number.
	pushl %eax

	# Move multiboot information pointer to virtual space and push it.
	addl $ASM_KM_VIRT_BASE, %ebx
	pushl %ebx

	# Set up proper paging tables.
	call early_init_kernel_paging

	# Call the global constructors.
	call _init

	# Initialize debug subsystem.
	call init_debug

	# Continue multiboot init in C code.
	call multiboot_x86_init

	# Hang if kernel_main unexpectedly returns.
	cli
2:
	hlt
	jmp 2b

.size _start, . - _start
