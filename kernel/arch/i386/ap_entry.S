/* ap_entry.S - entry code for AP CPUs */

#include <arch/gdt.h>
#include <arch/memlayout.h>
#include <arch/paging_types.h>
#include <arch/selectors.h>

/* This is the offset of the ap_entry.S code in virtual memory, as linked by the linker script. We
   use this offset to calculate addresses of things as if the executable was in low physical memory,
   at 0x8000 */
#define ASM_AP_ENTRY_OFFSET (ap_start - ASM_KM_PHYS_AP_ENTRY_BASE)

.code16
.global ap_start
ap_start:
	/* Immediately stop interrupts. We will enable them when we enter the scheduler on this CPU. */
	cli

	/* Clear the data segments. */
	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss

	/* Load our initial GDT, enable protected mode, and jump to 32-bit code. */
	lgdt	ap_start_gdtr - ASM_AP_ENTRY_OFFSET
	movl	%cr0, %eax
	orl		$CR0_PE, %eax
	movl	%eax, %cr0

	ljmpl	$KERNEL_CODE_SELECTOR, $(ap_start_32 - ASM_AP_ENTRY_OFFSET)

.code32
ap_start_32:
	/* Use the kernel data selector. */
	movw	$KERNEL_DATA_SELECTOR, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movw	$0, %ax
	movw	%ax, %fs
	movw	%ax, %gs

	/* Set the page directory */
	leal	(ap_entry_args + 8 - ASM_AP_ENTRY_OFFSET), %ebx
	movl	(%ebx), %eax
	movl	%eax, %cr3

	/* Set the paging bit */
	movl	%cr0, %eax
	orl		$CR0_PG, %eax
	movl	%eax, %cr0

	/* Set the global page enable bit */
	movl	%cr4, %eax
	orl		$CR4_PGE, %eax
	movl	%eax, %cr4

	/* Start using the stack we've been given. */
	leal	(ap_entry_args + 4 - ASM_AP_ENTRY_OFFSET), %eax
	movl	(%eax), %esp

	/* Push 0 for an unwinder. */
	pushl	$0

	/* Jump to C code. */
	call	*(ap_entry_args + 12 - ASM_AP_ENTRY_OFFSET)

	/* We should not return... Hang if we do. */
	cli
1:
	hlt
	jmp 1b

.global ap_entry_args
ap_entry_args:
	.long	0 /* vaddr_t stack_top */
	.long	0 /* vaddr_t stack_bottom */
	.long	0 /* paddr_t page_directory */
	.long	0 /* void (*entry)(void) */

.align 4
/* Initial GDT that only maps stuff for ring 0. */
ap_start_gdt:
	ASM_GDTE_NULL
	ASM_GDTE_ALL(ASM_GDTE_CODE32_FLAGS)
	ASM_GDTE_ALL(ASM_GDTE_DATA32_FLAGS)

ap_start_gdtr:
	.word	(ap_start_gdtr - ap_start_gdt - 1)
	.long	ap_start_gdt - ASM_AP_ENTRY_OFFSET
