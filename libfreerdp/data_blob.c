/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   DATA_BLOB routines

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2010

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

#include "mem.h"

#include "data_blob.h"

void data_blob_alloc(DATA_BLOB *data_blob, int length)
{
	data_blob->data = xmalloc(length);
	data_blob->length = length;
}

void data_blob_free(DATA_BLOB *data_blob)
{
	if (data_blob->data)
		xfree(data_blob->data);
	
	data_blob->length = 0;
}
