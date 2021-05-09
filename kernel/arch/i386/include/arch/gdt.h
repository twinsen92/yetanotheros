/* gdt.h - x86 GDT structures and declarations */
#ifndef ARCH_I386_GDT_H
#define ARCH_I386_GDT_H

#include <kernel/cdefs.h>
#include <arch/seg_types.h>

typedef struct
{
	uint16_t link;
	uint16_t _p0;

	uint32_t esp0;
	uint16_t ss0;
	uint16_t _p1;

	uint32_t esp1;
	uint16_t ss1;
	uint16_t _p2;

	uint32_t esp2;
	uint16_t ss2;
	uint16_t _p3;

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

	uint16_t es;
	uint16_t _p4;

	uint16_t cs;
	uint16_t _p5;

	uint16_t ss;
	uint16_t _p6;

	uint16_t ds;
	uint16_t _p7;

	uint16_t fs;
	uint16_t _p8;

	uint16_t gs;
	uint16_t _p9;

	uint16_t ldtr;
	uint16_t _p10;

	uint16_t debug_trap;
	uint16_t iopb;
} __attribute__((packed))
tss_t;

/* Segement bits. All bits are shifted 32 bits to the right. */

/* Access flags */

#define GDTE_BIT_ACCESSED 0x100 /* Entry has been accessed. Set by CPU. */
#define GDTE_BIT_CS_READABLE 0x200 /* Code segment is readable. */
#define GDTE_BIT_DS_WRITABLE 0x200 /* Data segment is writable. */
#define GDTE_BIT_DS_DOWN 0x400 /* Data segment grows downwards (limit greater than base.) */
#define GDTE_BIT_CS_CONFORM 0x400 /* Code segment can be accessed by lower rings. */
#define GDTE_BIT_CS_EXECUTABLE 0x800 /* Code segment is executable. */
#define GDTE_BIT 0x1000 /* GDT segment. */

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
	| GDTE_BIT_32 | GDTE_BIT_PAGE)

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

/* Segment selector macros */

#define SEG_SELECTOR(idx) (idx * sizeof(seg_t))
#define RING_SELECTOR(r) (r)
#define LDT_SELECTOR 0x0004
#define seg_selector_to_index(sel) ((sel & 0x07) / sizeof(seg_t))

/* YAOS2 specific stuff */

#define KERNEL_CODE_SELECTOR (SEG_SELECTOR(1) | RING_SELECTOR(0))
#define KERNEL_DATA_SELECTOR (SEG_SELECTOR(2) | RING_SELECTOR(0))
#define KERNEL_TSS_SELECTOR (SEG_SELECTOR(5) | RING_SELECTOR(0))
#define USER_CODE_SELECTOR (SEG_SELECTOR(3) | RING_SELECTOR(3))
#define USER_DATA_SELECTOR (SEG_SELECTOR(4) | RING_SELECTOR(3))
#define USER_TSS_SELECTOR (SEG_SELECTOR(5) | RING_SELECTOR(3))

#define YAOS2_GDT_NOF_ENTRIES 6

void init_gdt(void);

#endif
