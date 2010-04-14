/* -*- c-basic-offset: 8 -*-
   freerdp: A Remote Desktop Protocol client.
   memory functions
   Copyright (c) 2009-2010 Jay Sorg

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *
xmalloc(int size)
{
	void * mem;

	if (size < 1)
	{
		size = 1;
	}
	mem = malloc(size);
	if (mem == NULL)
	{
		perror("xmalloc");
	}
	return mem;
}

void *
xrealloc(void * oldmem, int size)
{
	void * mem;

	if (size < 1)
	{
		size = 1;
	}
	mem = realloc(oldmem, size);
	if (mem == NULL)
	{
		perror("xrealloc");
	}
	return mem;
}

void
xfree(void * mem)
{
	if (mem != NULL)
	{
		free(mem);
	}
}

char *
xstrdup(const char * s)
{
	char * mem = strdup(s);

	if (mem == NULL)
	{
		perror("strdup");
	}
	return mem;
}
