/* segmentation.c - x86 segmentation handling code */

#include <kernel/cdefs.h>
#include <kernel/interrupts.h>
#include <kernel/utils.h>
#include <arch/cpu.h>
#include <arch/gdt.h>
#include <arch/seg_types.h>

void init_gdt(void)
{
	x86_cpu_t *cpu = cpu_current();

	push_no_interrupts();

	/* Create the GDT. */
	cpu->gdt[0] = gdte_construct(0, 0, 0);
	cpu->gdt[1] = gdte_construct(0, 0xffffffff, GDTE_CODE32_FLAGS | SEG_BIT_RING(0));
	cpu->gdt[2] = gdte_construct(0, 0xffffffff, GDTE_DATA32_FLAGS | SEG_BIT_RING(0));
	cpu->gdt[3] = gdte_construct(0, 0xffffffff, GDTE_CODE32_FLAGS | SEG_BIT_RING(3));
	cpu->gdt[4] = gdte_construct(0, 0xffffffff, GDTE_DATA32_FLAGS | SEG_BIT_RING(3));
	cpu->gdt[5] = gdte_construct((uint32_t)&(cpu->tss), sizeof(tss_t), GDTE_TSS_FLAGS | SEG_BIT_RING(0));

	/* Load the GDT. */
	cpu->gdtr.size = sizeof(cpu->gdt);
	cpu->gdtr.offset = (vaddr32_t)(cpu->gdt);
	asm_ldtr(gdt, &cpu->gdtr);

	/* Jump into the kernel code segment. */
	asm volatile ("ljmp %0, $_with_kernel_cs; _with_kernel_cs: " : : "i" (KERNEL_CODE_SELECTOR));

	/* Use the kernel data segment. */
	asm volatile (
		"movl %0, %%eax\n"
		"movl %%eax, %%ds\n"
		"movl %%eax, %%es\n"
		"movl %%eax, %%fs\n"
		"movl %%eax, %%gs\n"
		"movl %%eax, %%ss\n"
		:
		: "i" (KERNEL_DATA_SELECTOR)
		: "eax"
	);

	/* Let's now fill out the TSS and load it. */
	memset_volatile(&(cpu->tss), 0, sizeof(tss_t));

	asm volatile (
		"movl %0, %%ebx\n"
		/* Fill out ss0:esp0 */
		"movl %%esp, %%eax\n"
		"movl %%eax, 4(%%ebx)\n"
		"movw %%ss, %%ax\n"
		"movw %%ax, 8(%%ebx)\n"
		/* Fill out iopb */
		"movw %2, %%ax\n"
		"movw %%ax, 102(%%ebx)\n"
		/* Fill out cr3 */
		"movl %%cr3, %%eax\n"
		"movl %%eax, 28(%%ebx)\n"
		/* Fill out eip */
		"leal _with_kernel_tss, %%eax\n"
		"movl %%eax, 32(%%ebx)\n"
		/* Fill out cs, ds, es, fs, gs, ss */
		"movw %%cs, %%ax\n"
		"movw %%ax, 76(%%ebx)\n"
		"movw %%ds, %%ax\n"
		"movw %%ax, 84(%%ebx)\n"
		"movw %%es, %%ax\n"
		"movw %%ax, 72(%%ebx)\n"
		"movw %%fs, %%ax\n"
		"movw %%ax, 88(%%ebx)\n"
		"movw %%gs, %%ax\n"
		"movw %%ax, 92(%%ebx)\n"
		"movw %%ss, %%ax\n"
		"movw %%ax, 80(%%ebx)\n"
		/* Load the TSS */
		"movl %1, %%eax\n"
		//"cli; 1: hlt; jmp 1b\n"
		"ltr %%ax\n"
		"_with_kernel_tss:"
		:
		: "r" (&(cpu->tss)), "i" (KERNEL_TSS_SELECTOR), "i" (sizeof(tss_t))
		: "eax", "ax", "ebx", "memory"
	);

	pop_no_interrupts();
}
