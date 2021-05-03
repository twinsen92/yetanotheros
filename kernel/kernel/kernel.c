#include <kernel/debug.h>

void kernel_main(void)
{
	debug_initialize();
	kdprintf("Hello, kernel World!\n");
}
