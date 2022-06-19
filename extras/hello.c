/* extras/hello.c - simple user program */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	char *buf;
	int fd, num;

	buf = malloc(256);
	memset(buf, 0, 256);

	printf("Hello, user World!\n");

	fd = open("/test.txt", 0);
	if (fd == -1)
		printf("Could not open /test.txt");
	else
	{
		num = read(fd, buf, 255);
		buf[num] = 0;
		close(fd);
		printf("Read from /test.txt: %s\n", buf);
	}

	free(buf);

	/*
	int *heap;

	for (int i = 0; i < 100; i++)
	{
		heap = malloc(sizeof(int));
		*heap = 0x12345678;
	}
	*/

	return 0;
}
