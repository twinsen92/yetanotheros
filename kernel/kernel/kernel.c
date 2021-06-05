/* kernel.c - main kernel compilation unit */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/test.h>
#include <kernel/devices/block.h>
#include <kernel/devices/pci.h>

noreturn kernel_main(void)
{
	kdprintf("Hello, kernel World!\n");

	/* Init non-critical shared subsystems. */
	init_bdev();
	init_pci();

	kalloc_test_main();
	while(1);
}
