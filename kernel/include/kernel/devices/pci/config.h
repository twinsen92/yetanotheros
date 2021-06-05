/* kernel/devices/pci/config.h */
#ifndef _KERNEL_DEVICES_PCI_CONFIG_H
#define _KERNEL_DEVICES_PCI_CONFIG_H

#include <kernel/cdefs.h>
#include <arch/kernel/portio.h>

/* PCI controller I/O ports. */
#define PCI_ADDRESS_PORT	0x0cf8
#define PCI_DATA_PORT		0x0cfc

/* Configuration offsets. */
#define CFG_VENDOR_ID		0x00
#define CFG_DEVICE_ID		0x02

#define CFG_COMMAND			0x04
#define CFG_STATUS			0x06

#define CFG_REVISION_ID		0x08
#define CFG_PROG_IF			0x09
#define CFG_SUBCLASS		0x0a
#define CFG_CLASS			0x0b

#define CFG_CACHE_LINE		0x0c
#define CFG_LATENCY			0x0d
#define CFG_HEADER_TYPE		0x0e
#define CFG_BIST			0x0f

#define CFG_BAR0			0x10
#define CFG_BAR1			0x14
#define CFG_BAR2			0x18
#define CFG_BAR3			0x1c
#define CFG_BAR4			0x20

#define CFG_INT_PIN			0x3d
#define CFG_INT_LINE		0x3c

/* Configuration values. */
#define CFG_INVALID_VENDOR	0xffff
#define CFG_HT_MASK			0x7f
#define CFG_DEFAULT			0x00
#define CFG_PCI2PCI			0x01
#define CFG_PCI2CB			0x02

#define CFG_MAX_FUNC		8 /* Max functions per device */
#define CFG_MAX_DEVICE		32 /* Max devices per bus */
#define CFG_MAX_BUS			256 /* Max buses */

/* Macros. */
#define cfg_is_multifunction(header_type) ((header_type & 0x80) != 0)
#define cfg_ht_equal(header_type, other) ((header_type & CFG_HT_MASK) == other)

static inline uint32_t config_read_dword(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
	uint32_t address;
	uint32_t lbus = (uint32_t)bus;
	uint32_t ldev = (uint32_t)dev;
	uint32_t lfunc = (uint32_t)func;
	uint16_t tmp = 0;

	/* Create a configuration address. */
	address = (uint32_t)((lbus << 16) | (ldev << 11) |
			  (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

	/* Write out the address. */
	pio_outl (PCI_ADDRESS_PORT, address);
	/* Read in the data. */
	tmp = pio_inl (PCI_DATA_PORT);
	return (tmp);
}

static inline uint16_t config_read_word(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
	uint32_t address;
	uint32_t lbus = (uint32_t) bus;
	uint32_t ldev = (uint32_t) dev;
	uint32_t lfunc = (uint32_t) func;
	uint16_t tmp = 0;

	/* Create configuration address. */
	address = (uint32_t) ((lbus << 16) | (ldev << 11) | (lfunc << 8)
			| (offset & 0xfc) | ((uint32_t) 0x80000000));

	/* Write out the address. */
	pio_outl(PCI_ADDRESS_PORT, address);
	/* Read in the data. */
	/* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
	tmp = (uint16_t) ((pio_inl(PCI_DATA_PORT) >> ((offset & 2) * 8)) & 0xffff);
	return (tmp);
}

static inline uint8_t config_read_byte(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t ldev = (uint32_t)dev;
	uint32_t lfunc = (uint32_t)func;
	uint8_t tmp = 0;

	/* Create a configuration address. */
	address = (uint32_t)((lbus << 16) | (ldev << 11) |
			  (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

	/* Write out the address. */
	pio_outl (PCI_ADDRESS_PORT, address);
	/* Read in the data. */
	/* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
	tmp = (uint8_t)((pio_inl (PCI_DATA_PORT) >> ((offset & 2) * 8)) & 0xff);
	return (tmp);
}

static inline void config_write_byte(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset,
	uint8_t value)
{
	uint32_t address;
	uint32_t lbus = (uint32_t) bus;
	uint32_t ldev = (uint32_t) dev;
	uint32_t lfunc = (uint32_t)func;
	uint32_t tmp = 0;

	/* Create a configuration address. */
	address = (uint32_t)((lbus << 16) | (ldev << 11) |
			  (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

	/* Write out the address. */
	pio_outl (PCI_ADDRESS_PORT, address);
	/* Read in the data, add tmp value */
	tmp = pio_inl (PCI_DATA_PORT) & ~(0xff << ((offset & 2) * 8));
	tmp |= ((uint32_t)value) << ((offset & 2) * 8);
	pio_outl (PCI_DATA_PORT, tmp);
}

static inline bool config_is_valid(uint8_t bus, uint8_t dev, uint8_t func)
{
	return config_read_word(bus, dev, func, CFG_VENDOR_ID) != CFG_INVALID_VENDOR;
}

#endif
