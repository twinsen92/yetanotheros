/* kernel.c - main kernel compilation unit */
#include <kernel/debug.h>

__attribute__((__noreturn__))
void kernel_main(void)
{
	kdprintf("Hello, kernel World!\n");
	while(1);
}
