/* vga_debug.c - VGA backed debug implementation */

#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/printf.h>
#include <arch/apic.h>
#include <arch/cpu.h>
#include <arch/interrupts.h>
#include <arch/memlayout.h>
#include <arch/vga.h>

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) km_vaddr(0xB8000);

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

/* VGA functions */

/* Initialize vga_printf */
void init_vga_printf(void)
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = VGA_MEMORY;
	for (size_t y = 0; y < VGA_HEIGHT; y++)
	{
		for (size_t x = 0; x < VGA_WIDTH; x++)
		{
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

static void terminal_set_panic(void)
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
}

static void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y)
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

static void terminal_putchar(char c)
{
	unsigned char uc = c;

	/* Handle newline */
	if (c == '\n')
	{
		terminal_column = 0;

		if (++terminal_row == VGA_HEIGHT)
			terminal_row = 0;

		return;
	}

	terminal_putentryat(uc, terminal_color, terminal_column, terminal_row);

	if (++terminal_column == VGA_WIDTH)
	{
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			terminal_row = 0;
	}
}

/* A printf to VGA buffer. Very reliable. */
int vga_printf(const char * __restrict format, ...)
{
	int ret;
	va_list parameters;

	va_start(parameters, format);
	ret = generic_printf(terminal_putchar, format, parameters);
	va_end(parameters);

	return ret;
}

/* A printf to VGA buffer. Very reliable. Version with va_list. */
int va_vga_printf(const char * __restrict format, va_list parameters)
{
	return generic_printf(terminal_putchar, format, parameters);
}

/* kernel/debug.h interface */

static void panic_enumerate(struct x86_cpu *cpu)
{
	/* Ignore inactive CPUs. */
	if (atomic_load(&(cpu->active)) == false)
		return;

	/* If we have a second, active CPU, we most likely have initialized LAPIC. Send a panic IPI. */

	lapic_ipi(cpu->lapic_id, INT_PANIC_IPI, 0);
	lapic_ipi_wait();
}

noreturn _kpanic(const char *reason, const char *file, unsigned int line)
{
	/* Stop interrupts. We're not nice with cpu.h, but at this point, who cares? This also helps
	   in a situation where cpu.h has not been initialized yet. */
	cpu_force_cli();

	/* Enumerate other CPUs and send panic IPIs to them. */
	cpu_enumerate_other_cpus(panic_enumerate);

	/* Reset to 0,0 and set an intimidating red colour. */
	terminal_set_panic();

	vga_printf("kernel: panic: %s\n", reason);
	vga_printf("at %s:%d\n", file, line);

	while (1)
	{
		cpu_relax();
	}

	__builtin_unreachable();
}
