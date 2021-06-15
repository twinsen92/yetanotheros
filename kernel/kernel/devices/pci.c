/* kernel/devices/pci.c - PCI registry and base driver definitions */
#include <kernel/cdefs.h>
#include <kernel/cpu.h>
#include <kernel/debug.h>
#include <kernel/thread.h>
#include <kernel/utils.h>
#include <kernel/devices/pci.h>
#include <kernel/devices/pci/config.h>
#include <arch/kernel/portio.h>

/* TODO: Replace ticks_mwait with thread_sleep (now that we use thread_mutex) */

/* PCI registry */
/* TODO: Use linked lists. */
#define MAX_PCI_FUNCTIONS 32
static struct thread_mutex pci_config_mutex;
static int next_pci_function = 0;
static struct pci_function pci_functions[MAX_PCI_FUNCTIONS];

static struct pci_function *register_function(void)
{
	if (next_pci_function == MAX_PCI_FUNCTIONS)
		kpanic("register_function(): too many PCI functions");

	return &pci_functions[next_pci_function++];
}

static inline void print_bar(uint32_t bar, int num)
{
	if ((bar & PCI_BAR_PORT_BIT) == 0)
	{
		if ((bar & PCI_BAR_MEMORY_MASK) != 0)
			kdprintf("  BAR%d: [memory] %p\n", num, bar & (~0xf));
	}
	else
	{
		if ((bar & PCI_BAR_PORT_MASK) != 0)
			kdprintf("  BAR%d: [port] %p\n", num, bar & (~0x3));
	}
}

static void visit_function(uint8_t bus, uint8_t dev, uint8_t func)
{
	struct pci_function *f;

	if (!config_is_valid(bus, dev, func))
		/* Function not present. */
		return;

	f = register_function();

	f->bus = bus;
	f->device = dev;
	f->function = func;

	f->id.vendor_id = config_read_word(bus, dev, func, CFG_VENDOR_ID);
	f->id.device_id = config_read_word(bus, dev, func, CFG_DEVICE_ID);

	f->revision = config_read_byte(bus, dev, func, CFG_REVISION_ID);
	f->prog_if = config_read_byte(bus, dev, func, CFG_PROG_IF);
	f->subclass = config_read_byte(bus, dev, func, CFG_SUBCLASS);
	f->class = config_read_byte(bus, dev, func, CFG_CLASS);

	f->int_line = config_read_byte(bus, dev, func, CFG_INT_LINE);
	f->int_pin = config_read_byte(bus, dev, func, CFG_INT_PIN);

	f->header_type = config_read_byte(bus, dev, func, CFG_HEADER_TYPE);

	kdprintf("PCI %x:%x.%x vendor %x, device %x, rev %x, pif %x, sub %x, cls %x\n",
		f->bus, f->device, f->function, f->id.vendor_id, f->id.device_id, f->revision, f->prog_if,
		f->subclass, f->class);

	kdprintf("             irq_pin %x, irq_line %x\n", f->int_pin, f->int_line);

	print_bar(config_read_dword(bus, dev, func, CFG_BAR0), 0);
	print_bar(config_read_dword(bus, dev, func, CFG_BAR1), 1);
	print_bar(config_read_dword(bus, dev, func, CFG_BAR2), 2);
	print_bar(config_read_dword(bus, dev, func, CFG_BAR3), 3);
	print_bar(config_read_dword(bus, dev, func, CFG_BAR4), 4);

	debug_flush();

	/* TODO: Handle PCI-PCI bridges. */
}

static void visit_device(uint8_t bus, uint8_t dev)
{
	uint8_t header_type = 0;
	uint8_t func = 0;

	if (!config_is_valid(bus, dev, 0))
		/* Device not present. */
		return;

	header_type = config_read_byte(bus, dev, 0, CFG_HEADER_TYPE);

	if (cfg_is_multifunction(header_type))
	{
		/* Multi-function device. */
		for (func = 0; func < CFG_MAX_FUNC; func++)
		{
			if (!config_is_valid(bus, dev, func))
				break;

			visit_function(bus, dev, func);
		}
	}
	else
	{
		/* Single-function device. */
		visit_function(bus, dev, 0);
	}
}

static void visit_bus(uint8_t bus)
{
	uint8_t dev;

	for(dev = 0; dev < CFG_MAX_DEVICE; dev++)
	{
		visit_device(bus, dev);
	}
}

/* kernel/devices/pci.h exports */

/* Initialize the PCI registry. This also enumerates PCI. */
void init_pci(void)
{
	uint8_t func, header_type;

	thread_mutex_create(&pci_config_mutex);
	next_pci_function = 0;
	kmemset(pci_functions, 0, sizeof(pci_functions));

	/* Enumerate PCI devices. */

	if (!config_is_valid(0, 0, 0))
		/* No PCI host controllers present? */
		return;

	/* Visit the first PCI host controller's bus. */
	visit_bus(0);

	header_type = config_read_byte(0, 0, 0, CFG_HEADER_TYPE);

	if (cfg_is_multifunction(header_type))
	{
		/* More PCI host controllers. */
		for (func = 1; func < CFG_MAX_FUNC; func++)
		{
			if (!config_is_valid(0, 0, func))
				break;

			visit_bus(func);
		}
	}
}

/* Register a driver with the PCI registry. The driver will be initialized for each matching
   device. */
void pci_register_driver(struct pci_driver *driver)
{
	struct pci_function *fun;
	struct pci_device_id *id, *sup;

	thread_mutex_acquire(&pci_config_mutex);

	/* Loop through functions without a driver. */
	for (uint i = 0; i < MAX_PCI_FUNCTIONS; i++)
	{
		fun = &(pci_functions[i]);

		if (fun->driver != NULL)
			continue;

		/* See if the function's device ID matches an ID supported by the driver. */

		id = &(fun->id);

		for (uint j = 0; j < driver->num_supported; j++)
		{
			sup = &(driver->supported[j]);

			if (id->vendor_id == sup->vendor_id && id->device_id == sup->device_id)
			{
				/* We have a match. Fill out the field in registry and initialize the driver. */
				fun->driver = driver;
				driver->init(driver, fun);
				break;
			}
		}
	}

	thread_mutex_release(&pci_config_mutex);
}

uint32_t pci_get_bar(struct pci_function *function, uint8_t index)
{
	if (!thread_mutex_held(&pci_config_mutex))
		kpanic("pci_get_bar(): config spinlock not held");

	/* Make sure there are BAR's */
	kassert((cfg_ht_equal(function->header_type, CFG_DEFAULT) && index <= 5) ||
			(cfg_ht_equal(function->header_type, CFG_PCI2PCI) && index <= 1));

	return config_read_dword(function->bus, function->device, function->function,
			CFG_BAR0 + index * sizeof (uint32_t));
}

uint8_t pci_get_int_line(struct pci_function *function)
{
	if (!thread_mutex_held(&pci_config_mutex))
		kpanic("pci_get_int_line(): config spinlock not held");

	return config_read_byte(function->bus, function->device, function->function,
			CFG_INT_LINE);
}

void pci_set_int_line(struct pci_function *function, uint8_t irq_line)
{
	if (!thread_mutex_held(&pci_config_mutex))
		kpanic("pci_set_int_line(): config spinlock not held");

	config_write_byte(function->bus, function->device, function->function,
			CFG_INT_LINE, irq_line);
}

uint8_t pci_get_status(struct pci_function *function)
{
	if (!thread_mutex_held(&pci_config_mutex))
		kpanic("pci_get_status(): config spinlock not held");

	return config_read_byte(function->bus, function->device, function->function,
			CFG_STATUS);
}

void pci_command(struct pci_function *function, uint8_t command)
{
	if (!thread_mutex_held(&pci_config_mutex))
		kpanic("pci_command(): config spinlock not held");

	config_write_byte(function->bus, function->device, function->function,
			CFG_COMMAND, command);
}
