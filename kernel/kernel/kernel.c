/* kernel.c - main kernel compilation unit */
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/test.h>

noreturn kernel_main(void)
{
	kdprintf("Hello, kernel World!\n");
	kalloc_test_main();
}
