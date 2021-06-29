/* cpu/segmentation.c - x86 segmentation handling code */

#include <kernel/cdefs.h>
#include <kernel/cpu.h>
#include <kernel/utils.h>
#include <arch/cpu.h>
#include <arch/cpu/gdt.h>
#include <arch/cpu/seg_types.h>
#include <arch/cpu/selectors.h>

/* Initializes the GDT for the current CPU. */
void init_gdt(void)
{
	struct x86_cpu *cpu = cpu_current();
	seg_t seg;

	push_no_interrupts();

	/* Create the GDT. */
	cpu->gdt[0] = gdte_construct(0, 0, 0);

	seg = gdte_construct(0, 0xffffffff, GDTE_CODE32_FLAGS | SEG_BIT_RING(0));
	cpu->gdt[seg_selector_to_index(KERNEL_CODE_SELECTOR)] = seg;

	seg = gdte_construct(0, 0xffffffff, GDTE_DATA32_FLAGS | SEG_BIT_RING(0));
	cpu->gdt[seg_selector_to_index(KERNEL_DATA_SELECTOR)] = seg;

	seg = gdte_construct(0, 0xffffffff, GDTE_CODE32_FLAGS | SEG_BIT_RING(3));
	cpu->gdt[seg_selector_to_index(USER_CODE_SELECTOR)] = seg;

	seg = gdte_construct(0, 0xffffffff, GDTE_DATA32_FLAGS | SEG_BIT_RING(3));
	cpu->gdt[seg_selector_to_index(USER_DATA_SELECTOR)] = seg;

	seg = gdte_construct((uint32_t)&(cpu->tss), sizeof(struct tss), GDTE_TSS_FLAGS | SEG_BIT_RING(0));
	cpu->gdt[seg_selector_to_index(KERNEL_TSS_SELECTOR)] = seg;

	/* Load the GDT. */
	cpu->gdtr.size = sizeof(cpu->gdt);
	cpu->gdtr.offset = (vaddr32_t)(cpu->gdt);
	asm_ldtr("gdt", &cpu->gdtr);

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

	/* Fill the TSS with zeroes. */
	memset_volatile(&(cpu->tss), 0, sizeof(struct tss));

	/* Set the address of the IO permission bitmap to the end of the segment. This disable port
	   operations in user mode. */
	cpu->tss.iopb = sizeof(struct tss);

	pop_no_interrupts();
}
