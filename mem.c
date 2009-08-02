
#include <stdio.h>
#include "rdesktop.h"

/* malloc; exit if out of memory */
void *
xmalloc(int size)
{
	void *mem = malloc(size);
	if (mem == NULL)
	{
		perror("xmalloc");
		exit(1);
	}
	return mem;
}

/* realloc; exit if out of memory */
void *
xrealloc(void *oldmem, size_t size)
{
	void *mem;

	if (size == 0)
		size = 1;
	mem = realloc(oldmem, size);
	if (mem == NULL)
	{
		perror("xrealloc");
		exit(1);
	}
	return mem;
}

/* free */
void
xfree(void *mem)
{
	free(mem);
}

/* strdup */
char *
xstrdup(const char *s)
{
	char *mem = strdup(s);
	if (mem == NULL)
	{
		perror("strdup");
		exit(1);
	}
	return mem;
}
