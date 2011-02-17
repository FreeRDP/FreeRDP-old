/*
   FreeRDP: A Remote Desktop Protocol client.
   DATA_BLOB routines

   Copyright 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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
