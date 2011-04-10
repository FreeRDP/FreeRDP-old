/*
   FreeRDP: A Remote Desktop Protocol client.
   stream routines

   Copyright (C) Jay Sorg 2011

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <freerdp/utils.h>
#include "frdp.h"
#include "stream.h"

int
stream_init(struct stream * st, size_t size)
{
	if (st == NULL)
	{
		return 1;
	}
	if (size > st->size)
	{
		if (st->data != NULL)
		{
			xfree(st->data);
		}
		st->data = (uint8 *) xmalloc(size);
		if (st->data == NULL)
		{
			return 1;
		}
		st->size = size;
	}
	st->p = st->data;
	st->end = st->data + st->size;
	return 0;
}

struct stream *
stream_new(int size)
{
	struct stream * st;

	st = (struct stream *) xmalloc(sizeof(struct stream));
	if (st == NULL)
	{
		return NULL;
	}
	memset(st, 0, sizeof(struct stream));
	if (stream_init(st, size) != 0)
	{
		xfree(st);
		return NULL;
	}
	return st;
}

int
stream_delete(struct stream * st)
{
	if (st != NULL)
	{
		if (st->data != NULL)
		{
			xfree(st->data);
		}
		xfree(st);
	}
	return 0;
}
