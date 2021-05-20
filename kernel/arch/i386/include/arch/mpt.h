/* mpt.h - MP tables scanning and parsing functions */
#ifndef ARCH_I386_MPT_H
#define ARCH_I386_MPT_H

#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <arch/mpt_types.h>

/* Scans the physical memory for the MP tables. Returns true if found. */
bool mpt_scan(void);

/* Pointer to the MP floating point structure. */
extern const struct mp_fps *mpt_fps;

/* Pointer to the MP configuration header. */
extern const struct mp_conf_header *mpt_header;

/* Walk through the MP table entries. */
void mpt_walk(void (*receiver)(const union mp_conf_entry *));

/* Enumerate and add CPUs. */
void mpt_enum_cpus(void);

/* Enumerate and add IOAPICs. */
void mpt_enum_ioapics(void);

/* Dump all MP entries to kdprintf. */
void mpt_dump(void);

#endif
