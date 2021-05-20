/* mpt_types.h - types used by the MP table */
#ifndef ARCH_I386_MPT_TYPES_H
#define ARCH_I386_MPT_TYPES_H

#include <kernel/cdefs.h>
#include <arch/memlayout.h>

/*  MP Floating Pointer Structure. */
#define MP_FPS_SIGNATURE     "_MP_"

packed_struct mp_fps
{
	char signature[4];
	uint32_t mp_conf_addr;
	uint8_t length;
	uint8_t revision;
	uint8_t checksum;
	uint8_t features[5];
};

/* MP Configuration Table. */
packed_struct mp_conf_header
{
	char signature[4];
	uint16_t length;
	uint8_t revision;
	uint8_t checksum;
	char oem_id[8];
	char prod_id[12];
	uint32_t oem_table_addr;
	uint16_t oem_table_length;
	uint16_t entry_count;
	uint32_t lapic_addr;
	uint16_t ext_length;
	uint8_t ext_checksum;
	uint8_t reserved;
};

/* MP Configuration Table entry types, structures and other constants. */
#define MP_CONF_PROC         0
#define MP_CONF_BUS          1
#define MP_CONF_IOAPIC       2
#define MP_CONF_IOINT        3 /* I/O Interrupt Assignment */
#define MP_CONF_LINT         4 /* Local Interrupt Assignment */

packed_struct mp_conf_proc
{
	uint8_t entry_type;
	uint8_t lapic_id;
	uint8_t lapic_ver;
	uint8_t cpu_flags;
	uint32_t cpu_signature;
	uint32_t features;
	uint32_t reserved1;
	uint32_t reserved2;
};

packed_struct mp_conf_bus
{
	uint8_t entry_type;
	uint8_t bus_id;
	char bus_type[6];
};

packed_struct mp_conf_ioapic
{
	uint8_t entry_type;
	uint8_t ioapic_id;
	uint8_t ioapic_ver;
	uint8_t ioapic_flags;
	uint32_t ioapic_addr;
};

#define MP_CONF_IOAPIC_EN    1

packed_struct mp_conf_ioint
{
	uint8_t entry_type;
	uint8_t int_type;
	uint16_t flags;
	uint8_t source_bus_id;
	uint8_t source_bus_irq;
	uint8_t dest_ioapic_id;
	uint8_t dest_ioapic_int;
};

packed_struct mp_conf_lint
{
	uint8_t entry_type;
	uint8_t int_type;
	uint16_t flags;
	uint8_t source_bus_id;
	uint8_t source_bus_irq;
	uint8_t dest_lapic_id;
	uint8_t dest_lapic_int;
};

packed_union mp_conf_entry
{
	uint8_t type;
	struct mp_conf_proc proc;
	struct mp_conf_bus bus;
	struct mp_conf_ioapic ioapic;
	struct mp_conf_ioint ioint;
	struct mp_conf_lint lint;
};

/* Scanning related constants. These are within the first 1M of memory, so we can use km_vaddr(). */

#define EBDA_START           km_vaddr(0x0009fc00)
#define EBDA_END             km_vaddr(0x000a0000)

#define BASE_MEM_TOP_START   km_vaddr(639 * 0x400)
#define BASE_MEM_TOP_END     km_vaddr(640 * 0x400)

#define BIOS_ROM_START       km_vaddr(0x000f0000)
#define BIOS_ROM_END         km_vaddr(0x00100000)

#endif
