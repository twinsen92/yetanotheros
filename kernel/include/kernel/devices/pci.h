/* kernel/devices/pci.h - PCI registry and base driver definitions */
#ifndef _KERNEL_DEVICES_PCI_H
#define _KERNEL_DEVICES_PCI_H

#include <kernel/addr.h>
#include <kernel/cdefs.h>

#define PCI_BAR_PORT_BIT     1

#define PCI_BAR_MEMORY_MASK  (~0xf)
#define PCI_BAR_PORT_MASK    (~0x3)

struct pci_driver;

/* pci.c */

struct pci_device_id
{
	uint16_t vendor_id;
	uint16_t device_id;
};

struct pci_function
{
	uint8_t bus;
	uint8_t device;
	uint8_t function;

	struct pci_device_id id;

	uint8_t revision;
	uint8_t prog_if;
	uint8_t subclass;
	uint8_t class;

	uint8_t header_type;
	uint8_t int_line;
	uint8_t int_pin;

	struct pci_driver *driver;
};

#define pci_bar_is_port(x) (x & 0x00000001)
#define pci_bar_get_port(x) (x & ~(0x00000003))
#define pci_bar_get_address(x) (x & ~(0x0000000f))

struct pci_driver
{
	struct pci_device_id *supported;
	uint num_supported;

	void *opaque;

	void (*init)(struct pci_driver *driver, struct pci_function *function);
};

/* PCI registry. */

/* Initialize the PCI registry. This also enumerates PCI. */
void init_pci(void);

/* Register a driver with the PCI registry. The driver will be initialized for each matching
   device. */
void pci_register_driver(struct pci_driver *driver);

/* PCI driver initialization interface. */

uint32_t pci_get_bar(struct pci_function *function, uint8_t index);
uint8_t pci_get_int_line(struct pci_function *function);
void pci_set_int_line(struct pci_function *function, uint8_t irq_line);
uint8_t pci_get_status(struct pci_function *function);
void pci_command(struct pci_function *function, uint8_t command);

#endif
