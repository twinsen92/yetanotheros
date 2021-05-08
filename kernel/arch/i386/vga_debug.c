/* vga_debug.c - VGA backed debug implementation */

#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <arch/boot_layout.h>
#include <arch/vga.h>

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) get_low_vaddr(0xB8000);

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

static size_t strlen(const char* str)
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

void debug_initialize(void)
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

static void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y)
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

static void terminal_putchar(char c)
{
	unsigned char uc = c;
	terminal_putentryat(uc, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH)
	{
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			terminal_row = 0;
	}
}

static bool print(const char* data, size_t length)
{
	const unsigned char* bytes = (const unsigned char*) data;
	for (size_t i = 0; i < length; i++)
		terminal_putchar(bytes[i]);
	return true;
}

int kdprintf(const char* restrict format, ...)
{
	va_list parameters;
	va_start(parameters, format);

	int written = 0;

	while (*format != '\0')
	{
		size_t maxrem = INT_MAX - written;

		if (format[0] != '%' || format[1] == '%')
		{
			if (format[0] == '%')
				format++;
			size_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			if (maxrem < amount)
			{
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, amount))
				return -1;
			format += amount;
			written += amount;
			continue;
		}

		const char* format_begun_at = format++;

		if (*format == 'c')
		{
			format++;
			char c = (char) va_arg(parameters, int /* char promotes to int */);
			if (!maxrem)
			{
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(&c, sizeof(c)))
				return -1;
			written++;
		}
		else if (*format == 's')
		{
			format++;
			const char* str = va_arg(parameters, const char*);
			size_t len = strlen(str);
			if (maxrem < len)
			{
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(str, len))
				return -1;
			written += len;
		}
		else
		{
			format = format_begun_at;
			size_t len = strlen(format);
			if (maxrem < len)
			{
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, len))
				return -1;
			written += len;
			format += len;
		}
	}

	va_end(parameters);
	return written;
}

__attribute__((__noreturn__))
void kabort(void)
{
	kdprintf("kernel: panic: kabort()\n");
	asm volatile("cli":::"memory");

	while (1)
	{
	}
	__builtin_unreachable();
}

__attribute__((__noreturn__))
void _kassert_failed(const char *expr, const char *file, unsigned int line)
{
	kdprintf("%s:%d:assertion %s failed\n", file, line, expr);
	kabort();
}
