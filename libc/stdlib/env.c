#include <stddef.h>
#include <string.h>

#include <yaos2/arch/syscall.h>

#include <yaos2/kernel/errno.h>
#include <yaos2/kernel/syscalls.h>

/* TODO: Add a lock. */

char *getenv(const char *name)
{
	/* TODO: Lazily copy the initial environment to our own structures. */
	size_t len;
	int i;
	const char **envt;
	const char *env;

	if (!name)
		return NULL;

	len = strlen(name);
	i = 0;
	envt = (const char **)syscall0(SYSCALL_GETENVPTR);

	while (1)
	{
		env = envt[i];

		if (env == NULL)
			break;

		/*
		 * Try to match the len first characters in this env string. Check if the next character
		 * is an equals sign. If not, this is not the variable we're looking for.
		 */
		if (strncmp(env, name, len) == 0 && env[len] == '=')
			return (char*)(env + len + 1);

		i++;
	}

	return NULL;
}
