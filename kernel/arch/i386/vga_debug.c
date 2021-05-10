/* vga_debug.c - VGA backed debug implementation */

#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <arch/memlayout.h>
#include <arch/cpu.h>
#include <arch/vga.h>

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) km_vaddr(0xB8000);

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

/* Utility functions */
static const char *numcodes32 =
		"zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";

static size_t strlen(const char* str)
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

static char *unsigned_itoa(unsigned int value, char *str, int base)
{
	char* ptr = str, *ptr1 = str, tmp_char;
	unsigned int tmp_value;

	do
	{
		tmp_value = value;
		value /= base;
		*ptr++ = numcodes32[35 + (tmp_value - value * base)];
	} while (value);

	*ptr-- = '\0';

	while (ptr1 < ptr)
	{
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}

	return str;
}

static char *itoa(int value, char *str, int base)
{
	char* ptr = str, *ptr1 = str, tmp_char;
	int tmp_value;

	if (base < 2 && base > 32)
	{
		*str = '\0';
		return "bad base";
	}

	if (base != 10)
		return unsigned_itoa(value, str, base);

	do
	{
		tmp_value = value;
		value /= base;
		*ptr++ = numcodes32[35 + (tmp_value - value * base)];
	} while (value);

	// Apply negative sign
	if (tmp_value < 0)
		*ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr)
	{
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}

	return str;
}

/* VGA functions */

void init_debug(void)
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

/* kernel/debug.h interface */

static bool print(const char* data, size_t length)
{
	const unsigned char* bytes = (const unsigned char*) data;
	for (size_t i = 0; i < length; i++)
		terminal_putchar(bytes[i]);
	return true;
}

int kdprintf(const char* restrict format, ...)
{
	char buf[32];
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
		else if (*format == 'd')
		{
			format++;
			int i = va_arg(parameters, int);
			char *str = itoa(i, buf, 10);
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
		else if (*format == 'u')
		{
			format++;
			unsigned int i = va_arg(parameters, unsigned int);
			char *str = unsigned_itoa(i, buf, 10);
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
		else if (*format == 'x')
		{
			format++;
			unsigned int i = va_arg(parameters, unsigned int);
			/* TODO: What about uint64_t? */
			char *str = unsigned_itoa(i, buf, 16);
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
		else if (*format == 'p')
		{
			format++;
			vaddr_t p = va_arg(parameters, vaddr_t);
			/* TODO: This won't work in 64-bit mode. */
			kassert(sizeof(unsigned int) == sizeof(vaddr_t));
			char *str = unsigned_itoa((unsigned int)p, buf, 16);
			size_t len = strlen(str);
			int zeroes = sizeof(vaddr_t) * 2 - len;
			if (maxrem < len + zeroes)
			{
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			for (int i = 0; i < zeroes; i++)
				if (!print("0", 1))
					return -1;
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

noreturn _kpanic(const char *reason, const char *file, unsigned int line)
{
	x86_cpu_t *cpu;

	/* Stop interrupts. We're not nice with cpu.h, but at this point, who cares? This also helps
	   in a situation where cpu.h has not been initialized yet. */
   	cpu_force_cli();

	/* Reset to 0,0 and set an intimidating red colour. */
	terminal_set_panic();

	kdprintf("kernel: panic: %s\n", reason);
	kdprintf("at %s:%d\n", file, line);

	/* See if we can print additional info. */
	cpu = cpu_current_or_null();

	if (cpu != NULL && cpu->isr_frame != NULL)
	{
		kdprintf("in interrupt %x", cpu->isr_frame->int_no);
		kdprintf("error_code=%x\n", cpu->isr_frame->error_code);
		kdprintf("EIP=%p, CS=%x, EFLAGS=%x\n", cpu->isr_frame->eip, cpu->isr_frame->cs, cpu->isr_frame->eflags);
		kdprintf("ESP=%p, SS=%x\n", cpu->isr_frame->esp, cpu->isr_frame->ss);
	}

	while (1)
	{
		cpu_relax();
	}

	__builtin_unreachable();
}
