/* multiboot_init.c - multiboot x86 init */

#include <kernel/cdefs.h>
#include <kernel/boot/multiboot.h>
#include <arch/init.h>

__attribute__((__noreturn__))
void multiboot_x86_init(struct multiboot_info *info, uint32_t magic)
{
	generic_x86_init();
}
