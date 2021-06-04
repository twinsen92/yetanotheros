/* mpt.c - MP tables scanning and parsing functions */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/init.h>
#include <kernel/utils.h>
#include <arch/cpu.h>
#include <arch/mpt.h>
#include <arch/mpt_types.h>

/* Pointer to the MP floating point structure. */
const struct mp_fps *mpt_fps = NULL;

/* Pointer to the MP configuration header. */
const struct mp_conf_header *mpt_header = NULL;

static bool scan_range(vaddr_t start, vaddr_t end)
{
	const struct mp_fps *fps;

	/* Loop while an mp_fps object fits between start and end. */
	while (start < end - sizeof(struct mp_fps) + 1)
	{
		fps = start++;

		if (kstrncmp(fps->signature, MP_FPS_SIGNATURE, sizeof(fps->signature)))
		{
			mpt_fps = fps;
			mpt_header = vm_map_walk(fps->mp_conf_addr, true);
			return true;
		}
	}

	return false;
}

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

/* Walk through the MP table entries. */
void mpt_walk(void (*receiver)(const union mp_conf_entry *))
{
	const uint8_t *ptr;
	const union mp_conf_entry *entry;

	/* Let's assume the MP tables have been found. */
	kassert(mpt_header);

	ptr = (const uint8_t *)(mpt_header) + sizeof(struct mp_conf_header);

	for (int i = 0; i < mpt_header->entry_count; i++)
	{
		entry = (const union mp_conf_entry *)ptr;

		receiver(entry);

		switch(entry->type)
		{
		case MP_CONF_PROC:
			ptr += sizeof(struct mp_conf_proc);
			break;
		case MP_CONF_BUS:
			ptr += sizeof(struct mp_conf_bus);
			break;
		case MP_CONF_IOAPIC:
			ptr += sizeof(struct mp_conf_ioapic);
			break;
		case MP_CONF_IOINT:
			ptr += sizeof(struct mp_conf_ioint);
			break;
		case MP_CONF_LINT:
			ptr += sizeof(struct mp_conf_lint);
			break;
		default:
			kpanic("mpt_walk(): unknown entry");
		}
	}
}

static void mpt_enum_cpus_receiver(const union mp_conf_entry *entry)
{
	if (entry->type != MP_CONF_PROC)
		return;

	cpu_add(entry->proc.lapic_id);
}

/* Enumerate and add CPUs. */
void mpt_enum_cpus(void)
{
	/* Yeah, this will only be done in initialization... */
	kassert(is_yaos2_initialized() == false);

	mpt_walk(mpt_enum_cpus_receiver);
}

static uint8_t ioapic_offset;

static void mpt_enum_ioapics_receiver(const union mp_conf_entry *entry)
{
	const struct mp_conf_ioapic *ioapic = &(entry->ioapic);

	if (entry->type != MP_CONF_IOAPIC)
		return;

	if(ioapic->ioapic_flags & MP_CONF_IOAPIC_EN)
	{
		/* TODO: Find out how many interrupts the I/O APIC handles. */
		ioapic_register((paddr_t)ioapic->ioapic_addr, ioapic->ioapic_id, ioapic_offset, 24);
		ioapic_offset += 24;
	}
}

/* Enumerate and add IOAPICs. */
void mpt_enum_ioapics(void)
{
	/* Yeah, this will only be done in initialization... */
	kassert(is_yaos2_initialized() == false);

	ioapic_offset = 0;

	mpt_walk(mpt_enum_ioapics_receiver);
}

static const char *pci_pin = "abcd";

static void mpt_dump_receiver(const union mp_conf_entry *entry)
{
	const struct mp_conf_proc *proc = &(entry->proc);
	const struct mp_conf_bus *bus = &(entry->bus);
	const struct mp_conf_ioapic *ioapic = &(entry->ioapic);
	const struct mp_conf_ioint *ioint = &(entry->ioint);
	const struct mp_conf_lint *lint = &(entry->lint);
	char str[16];

	kmemset(str, 0, sizeof(str));

	switch(entry->type)
	{
	case MP_CONF_PROC:
		kdprintf("MP proc: id=%d, ver=%d, flags=%x, sig=%x, features=%x\n",
			proc->lapic_id, proc->lapic_ver, proc->cpu_flags, proc->cpu_signature, proc->features);
		break;
	case MP_CONF_BUS:
		/* TODO: Use this information to configure IO APICs. */
		kstrncpy(str, bus->bus_type, sizeof(bus->bus_type));
		kdprintf("MP bus: id=%d, type=%s\n", bus->bus_id, str);
		break;
	case MP_CONF_IOAPIC:
		kdprintf("MP ioapic: id=%d, ver=%d, flags=%x, addr=%x, off=%x, sz=%d\n",
			ioapic->ioapic_id, ioapic->ioapic_ver, ioapic->ioapic_flags, ioapic->ioapic_addr,
			ioapic_offset, 24);
		break;
	case MP_CONF_IOINT:
		/* TODO: Use this information to configure IO APICs. */
		kdprintf("MP ioint: t=%d, f=%x, sb=%d, sirq=%d, did=%d, dirq=%d, PCI s=%d, p=%c\n",
			ioint->int_type, ioint->flags, ioint->source_bus_id, ioint->source_bus_irq,
			ioint->dest_ioapic_id, ioint->dest_ioapic_int, (ioint->source_bus_irq & 0x7c) >> 2,
			pci_pin[ioint->source_bus_irq & 0x03]);
		break;
	case MP_CONF_LINT:
		/* TODO: Use this information to configure local APICs. */
		kdprintf("MP lint: t=%d, f=%x, sb=%d, sirq=%d, did=%d, dirq=%d\n", lint->int_type,
			lint->flags, lint->source_bus_id, lint->source_bus_irq, lint->dest_lapic_id,
			lint->dest_lapic_int);
		break;
	}
}

/* Dump all MP entries to kdprintf. */
void mpt_dump(void)
{
	mpt_walk(mpt_dump_receiver);
}
