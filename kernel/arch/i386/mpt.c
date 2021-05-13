/* mpt.c - MP tables scanning and parsing functions */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/init.h>
#include <kernel/utils.h>
#include <arch/apic.h>
#include <arch/cpu.h>
#include <arch/mpt.h>
#include <arch/mpt_types.h>

static mpt_info_t info = {
	NULL
};
static mpt_info_t *info_ptr = NULL;
static uint8_t ioapic_offset = 0;
const char *pci_pin = "abcd";

static int visit_entry(const uint8_t *addr)
{
	const struct mp_conf_proc *proc = (const struct mp_conf_proc *)addr;
	const struct mp_conf_bus *bus = (const struct mp_conf_bus *)addr;
	const struct mp_conf_ioapic *ioapic = (const struct mp_conf_ioapic *)addr;
	__unused const struct mp_conf_ioint *ioint = (const struct mp_conf_ioint *)addr;
	__unused const struct mp_conf_lint *lint = (const struct mp_conf_lint *)addr;
	uint8_t type = *addr;
	char str[16];

	kmemset(str, 0, sizeof(str));

	switch(type)
	{
	case MP_CONF_PROC:
		kdprintf("MP proc: id=%d, ver=%d, flags=%x, sig=%x, features=%x\n",
			proc->lapic_id, proc->lapic_ver, proc->cpu_flags, proc->cpu_signature, proc->features);
		cpu_add(proc->lapic_id);
		return sizeof(struct mp_conf_proc);
	case MP_CONF_BUS:
		/* TODO: Use this information to configure IO APICs. */
		kstrncpy(str, bus->bus_type, sizeof(bus->bus_type));
		kdprintf("MP bus: id=%d, type=%s\n", bus->bus_id, str);
		return sizeof(struct mp_conf_bus);
	case MP_CONF_IOAPIC:
		kdprintf("MP ioapic: id=%d, ver=%d, flags=%x, addr=%x, off=%x, sz=%d\n",
			ioapic->ioapic_id, ioapic->ioapic_ver, ioapic->ioapic_flags, ioapic->ioapic_addr,
			ioapic_offset, 24);

		if(ioapic->ioapic_flags & MP_CONF_IOAPIC_EN)
		{
			/* TODO: Find out how many interrupts the I/O APIC handles. */
			ioapic_register((paddr_t)ioapic->ioapic_addr, ioapic->ioapic_id, ioapic_offset, 24);
			ioapic_offset += 24;
		}

		return sizeof(struct mp_conf_ioapic);
	case MP_CONF_IOINT:
		/* TODO: Use this information to configure IO APICs. */
		/*
		kdprintf("MP ioint: t=%d, f=%x, sb=%d, sirq=%d, did=%d, dirq=%d, PCI s=%d, p=%c\n",
			ioint->int_type, ioint->flags, ioint->source_bus_id, ioint->source_bus_irq,
			ioint->dest_ioapic_id, ioint->dest_ioapic_int, (ioint->source_bus_irq & 0x7c) >> 2,
			pci_pin[ioint->source_bus_irq & 0x03]);
		*/
		return sizeof(struct mp_conf_ioint);
	case MP_CONF_LINT:
		/* TODO: Use this information to configure local APICs. */
		/*
		kdprintf("MP lint: t=%d, f=%x, sb=%d, sirq=%d, did=%d, dirq=%d\n", lint->int_type, lint->flags, lint->source_bus_id,
			lint->source_bus_irq, lint->dest_lapic_id, lint->dest_lapic_int);
		*/
		return sizeof(struct mp_conf_lint);
	}

	/* This code should not be reachable. */
	kdprintf("MP: Unknown entry type %d\n", type);
	kabort();

	return 0;
}

static void parse_mp_conf(const struct mp_conf_header *header)
{
	const uint8_t *ptr;
	int entry;

	info.lapic_base = vm_map_walk(header->lapic_addr, true);
	kdprintf("MP LAPIC phys=%x\n", header->lapic_addr);
	ptr = (const uint8_t *)(header) + sizeof(struct mp_conf_header);

	for (entry = 0; entry < header->entry_count; entry++)
		ptr += visit_entry(ptr);
}

static bool scan_range(vaddr_t start, vaddr_t end)
{
	const struct mp_fps *fps;
	/* Loop while an mp_fps object fits between start and end. */
	while (start < end - sizeof(struct mp_fps) + 1)
	{
		fps = start++;

		if (kstrncmp(fps->signature, MP_FPS_SIGNATURE, sizeof(fps->signature)))
		{
			parse_mp_conf(km_vaddr(fps->mp_conf_addr));
			info_ptr = &info;
			return true;
		}
	}

	return false;
}

/* mpt.h exports */

/* Scans the physical memory for the MP tables. Returns true if found. */
bool mpt_scan(void)
{
	kassert(is_yaos2_initialized() == false);

	if (scan_range(EBDA_START, EBDA_END))
		return true;

	if (scan_range(BASE_MEM_TOP_START, BASE_MEM_TOP_END))
		return true;

	if (scan_range(BIOS_ROM_START, BIOS_ROM_END))
		return true;

	return false;
}

/* Gets the pointer to the parsed MP table info. */
const mpt_info_t *mpt_get_info(void)
{
	return info_ptr;
}
