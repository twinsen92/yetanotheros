/* early_debug.c - early debug functions */
/* Only other objects in arch/i386/boot are available in this compilation unit. */

#include <kernel/cdefs.h>
#include <arch/vga.h>

static uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;

noreturn early_kassert_failed(void)
{
	asm volatile("cli":::"memory");

	VGA_MEMORY[0] = vga_entry('E', vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));

	while (1)
	{
	}
	__builtin_unreachable();
}
