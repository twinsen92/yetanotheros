/* kernel.c - main kernel compilation unit */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/test.h>
#include <kernel/devices/block.h>

noreturn kernel_main(void)
{
	kdprintf("Hello, kernel World!\n");

	/* Init non-critical shared subsystems. */
	init_bdev();

	kalloc_test_main();
	while(1);
}
