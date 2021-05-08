/* segmentation.c - x86 segmentation handling code */

#include <kernel/cdefs.h>
#include <kernel/utils.h>
#include <arch/segmentation.h>

volatile tss_t cpu_tss;

static seg_t _cpu_gdt[6];
seg_t *cpu_gdt = _cpu_gdt;

gdtr_t cpu_gdtr = {
	sizeof(_cpu_gdt), (vaddr32_t)_cpu_gdt
};

void init_segmentation(void)
{
	/* Create the GDT. */
	cpu_gdt[0] = seg_construct(0, 0, 0);
	cpu_gdt[1] = seg_construct(0, 0xffffffff, GDT_CODE32_FLAGS | SEG_BIT_RING(0));
	cpu_gdt[2] = seg_construct(0, 0xffffffff, GDT_DATA32_FLAGS | SEG_BIT_RING(0));
	cpu_gdt[3] = seg_construct(0, 0xffffffff, GDT_CODE32_FLAGS | SEG_BIT_RING(3));
	cpu_gdt[4] = seg_construct(0, 0xffffffff, GDT_DATA32_FLAGS | SEG_BIT_RING(3));
	cpu_gdt[5] = seg_construct((uint32_t)&cpu_tss, sizeof(tss_t), GDT_TSS_FLAGS | SEG_BIT_RING(0));

	/* Load the GDT. */
	asm volatile ("lgdt (%0)" : : "r" (&cpu_gdtr) : "memory");

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
	memset_volatile(&cpu_tss, 0, sizeof(tss_t));

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
		: "r" (&cpu_tss), "i" (KERNEL_TSS_SELECTOR), "i" (sizeof(tss_t))
		: "eax", "ax", "ebx", "memory"
	);

}
