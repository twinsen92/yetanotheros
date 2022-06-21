/* extras/hello.c - simple user program */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void getenv_and_print(const char *name)
{
	char *value = getenv(name);

	if (value)
		printf("%s=%s\n", name, value);
	else
		printf("%s is not defined\n", name);
}

int main(int argc, char **argv)
{
	char *buf;
	FILE *f;
	int num;

	buf = malloc(256);
	memset(buf, 0, 256);

	printf("Hello, user World!\n");

	f = fopen("/test.txt", "r");
	if (f == NULL)
		printf("Could not open /test.txt");
	else
	{
		num = fread(buf, 1, 255, f);
		buf[num] = 0;
		printf("Read from /test.txt: %s\n", buf);
		fseek(f, 6, SEEK_SET);
		memset(buf, 0, 256);
		num = fread(buf, 1, 255, f);
		buf[num] = 0;
		printf("Read from /test.txt after fseek: %s\n", buf);

		fclose(f);
	}

	pid_t pid = fork();

	if (pid == 0)
	{
		printf("In child process buf: %s\n", buf);
		free(buf);
		exit(2137);
	}

	printf("Created child process %d\n", pid);

	int status = 0;
	pid = wait(&status);
	printf("Collected child process %d with status %d\n", pid, status);

	printf("Again: %s\n", buf);

	getenv_and_print("TEST");
	getenv_and_print("ASDF");

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
