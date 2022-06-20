#include <string.h>

size_t strlen(const char* str)
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

const char *strchr(const char *str, int chr)
{
	while(*str != '\0')
	{
		if (*str == chr)
			return str;
		str++;
	}

	return NULL;
}

char *strcat(char *to, const char *from)
{
	strcpy(to + strlen(to), from);
	return to;
}

char *strcpy(char *to, const char *from)
{
	strncpy(to, from, strlen(from));
	return to;
}

char *strncpy(char *to, const char *from, size_t num)
{
	char *ptr = to;

	while(*from != '\0' && num > 0)
	{
		*ptr = *from;
		ptr++;
		from++;
		num--;
	}

	*ptr = '\0';

	return to;
}

int strcmp(const char *s1, const char *s2)
{
	size_t s1_len = strlen(s1);
	size_t s2_len = strlen(s2);

	if (s1_len > s2_len)
		return 1;

	if (s1_len < s2_len)
		return -1;

	for (size_t i = 0; i < s1_len; i++)
	{
		if (s1[i] < s2[i])
			return -1;
		else if (s1[i] > s2[i])
			return 1;
	}

	return 0;
}

int strncmp(const char *s1, const char *s2, size_t num)
{
	for (size_t i = 0; i < num; i++)
	{
		if (s1[i] < s2[i])
			return -1;
		else if (s1[i] > s2[i])
			return 1;
	}

	return 0;
}

char *strstr(const char *s1, const char *s2)
{
	size_t len1, len2, i;

	if (s1 == NULL || s2 == NULL)
		return NULL;

	len1 = strlen(s1);
	len2 = strlen(s2);

	if (len2 == 0)
		return (char*)s1;

	if (len1 < len2)
		return NULL;

	for (i = 0; i < len1; i++)
	{
		/* No point iterating further if the remainder of s1 is shorter than s2. */
		if (len1 - i < len2)
			break;

		if (strncmp(s1 + i, s2, len2) == 0)
			return (char*)(s1 + i);
	}

	return NULL;
}
