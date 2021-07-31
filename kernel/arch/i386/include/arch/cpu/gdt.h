/* cpu/gdt.h - x86 GDT structures and declarations */
#ifndef ARCH_I386_CPU_GDT_H
#define ARCH_I386_CPU_GDT_H

#include <arch/cpu/seg_types.h>

/* Segement bits. All bits are shifted 32 bits to the right. */

/* Access flags */

#define GDTE_BIT_ACCESSED 0x100 /* Entry has been accessed. Set by CPU. */
#define GDTE_BIT_CS_READABLE 0x200 /* Code segment is readable. */
#define GDTE_BIT_DS_WRITABLE 0x200 /* Data segment is writable. */
#define GDTE_BIT_DS_DOWN 0x400 /* Data segment grows downwards (limit greater than base.) */
#define GDTE_BIT_CS_CONFORM 0x400 /* Code segment can be accessed by lower rings. */
#define GDTE_BIT_CS_EXECUTABLE 0x800 /* Code segment is executable. */
#define GDTE_BIT 0x1000 /* GDT segment. */
#define TSSD_BIT_BUSY 0x200 /* TSS descriptor busy flag. */

/* Type flags */

#define GDTE_BIT_SOFTWARE 0x100000 /* Segment is for software information only. */
#define GDTE_BIT_64 0x200000 /* 64-bit (long) mode segment. */
#define GDTE_BIT_32 0x400000 /* 32-bit mode segment. */
#define GDTE_BIT_PAGE 0x800000 /* Segment has page granularity. */

/* Common flag sets */

#define GDTE_CODE32_FLAGS (GDTE_BIT_CS_READABLE | GDTE_BIT_CS_EXECUTABLE | GDTE_BIT \
	| SEG_BIT_PRESENT | GDTE_BIT_32 | GDTE_BIT_PAGE)
#define GDTE_DATA32_FLAGS (GDTE_BIT_DS_WRITABLE | GDTE_BIT | SEG_BIT_PRESENT | GDTE_BIT_32 \
	| GDTE_BIT_PAGE)
#define GDTE_TSS_FLAGS (GDTE_BIT_ACCESSED | GDTE_BIT_CS_EXECUTABLE | SEG_BIT_PRESENT \
	| GDTE_BIT_32)

/* Control register segmentation related bits. */

#define CR0_PE				0x00000001 /* Protected mode enable */

#ifdef __ASSEMBLER__

#define ASM_GDTE_CODE32_FLAGS (GDTE_BIT_CS_READABLE + GDTE_BIT_CS_EXECUTABLE + GDTE_BIT \
	+ SEG_BIT_PRESENT + GDTE_BIT_32 + GDTE_BIT_PAGE)
#define ASM_GDTE_DATA32_FLAGS (GDTE_BIT_DS_WRITABLE + GDTE_BIT + SEG_BIT_PRESENT + GDTE_BIT_32 \
	+ GDTE_BIT_PAGE)
#define ASM_GDTE_TSS_FLAGS (GDTE_BIT_ACCESSED + GDTE_BIT_CS_EXECUTABLE + SEG_BIT_PRESENT \
	+ GDTE_BIT_32 + GDTE_BIT_PAGE)

#define ASM_GDTE_NULL .long 0, 0 /* Null descriptor. */
#define ASM_GDTE_ALL(flags) .long 0x0000FFFF, (0x000F0000 + (flags)) /* 4 GiB segment - "all" */

#endif

#ifndef __ASSEMBLER__

#include <kernel/cdefs.h>

packed_struct tss
{
	uint16_t link, :16;

	uint32_t esp0;
	uint16_t ss0, :16;

	uint32_t esp1;
	uint16_t ss1, :16;

	uint32_t esp2;
	uint16_t ss2, :16;

	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;

	uint16_t es, :16;
	uint16_t cs, :16;
	uint16_t ss, :16;
	uint16_t ds, :16;
	uint16_t fs, :16;
	uint16_t gs, :16;
	uint16_t ldtr, :16;

	uint16_t debug_trap;
	uint16_t iopb;
};

/* Macros and inlines. */

static inline seg_t gdte_construct(uint32_t base, uint32_t limit, uint32_t flags) {
	seg_t seg = 0;

	/* Modify upper 32 bits. */
	seg |= flags;
	seg |= limit & 0x000F0000; /* limit bits 19:16 */
	seg |= (base >> 16) & 0x000000FF; /* base bits 23:16 */
	seg |= base & 0xFF000000; /* base bits 31:24 */

	seg <<= 32;

	/* Modify lower 32 bits. */
	seg |= base << 16; /* base bits 15:0 */
	seg |= limit & 0x0000FFFF; /* limit bits 15:0 */

	return seg;
}

static inline seg_t gdte_clear_flag(seg_t seg, uint32_t flags)
{
	seg_t ret = 0;
	uint32_t upper = (uint32_t)(seg >> 32);
	uint32_t lower = (uint32_t)(seg & 0xffffffff);

	upper &= ~flags;

	ret = (seg_t)upper;
	ret <<= 32;
	ret |= lower;

	return ret;
}

#define seg_selector_to_index(sel) ((sel & ~0x07) / sizeof(seg_t))

/* YAOS2 specific stuff */

/* Initializes the GDT for the current CPU. */
void init_gdt(void);

#endif

#endif
