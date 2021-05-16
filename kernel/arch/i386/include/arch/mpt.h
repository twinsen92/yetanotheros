/* mpt.h - MP tables scanning and parsing functions */
#ifndef ARCH_I386_MPT_H
#define ARCH_I386_MPT_H

#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <arch/mpt_types.h>

struct mpt_info
{
	vaddr_t lapic_base;
};

/* Scans the physical memory for the MP tables. Returns true if found. */
bool mpt_scan(void);

/* Gets the pointer to the parsed MP table info. */
const struct mpt_info *mpt_get_info(void);

#endif
