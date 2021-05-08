/* seg_types.h - x86 segmentation generic structures and declarations */
#ifndef ARCH_I386_SEG_TYPES_H
#define ARCH_I386_SEG_TYPES_H

#include <kernel/addr.h>
#include <kernel/cdefs.h>

/* Structures */

typedef struct
{
	uint16_t size;
	vaddr32_t offset;
} __attribute__((packed))
dtr_t;

typedef uint64_t seg_t;

/* Segement bits. All bits are shifted 32 bits to the right. */

#define SEG_BIT_RING(r) ((r) << 13) /* Ring macro. */
#define SEG_BIT_PRESENT 0x8000 /* Segment is present and usable. */

#define asm_ldtr(type, ptr) asm volatile ("l" #type " (%0)" : : "r" (ptr) : "memory")

#endif
