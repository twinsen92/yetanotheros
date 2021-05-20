/* debug.c - x86 kernel/debug.h implementation */
#include <kernel/cdefs.h>
#include <kernel/printf.h>
#include <kernel/spinlock.h>
#include <arch/vga.h>

static struct spinlock kdprintf_spinlock;

/* Initializes the debug subsystem in the kernel. Call this early in initialization. */
void init_debug(void)
{
	init_vga_printf();
	spinlock_create(&kdprintf_spinlock, "kdprintf spinlock");
}

/* Debug formatted printer. */
int kdprintf(const char * restrict format, ...)
{
	int ret;
	va_list parameters;

	va_start(parameters, format);
	spinlock_acquire(&kdprintf_spinlock);
	ret = va_vga_printf(format, parameters);
	spinlock_release(&kdprintf_spinlock);
	va_end(parameters);

	return ret;
}
