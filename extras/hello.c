/* extras/hello.c - simple user program */

#include <unistd.h>

int main(int argc, char **argv)
{
	int *heap;

	for (int i = 0; i < 100; i++)
	{
		heap = sbrk(sizeof(int));
		*heap = 0x12345678;
	}

	return 0;
}
