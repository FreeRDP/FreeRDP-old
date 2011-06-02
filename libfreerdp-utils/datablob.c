/*
   FreeRDP: A Remote Desktop Protocol client.
   DATABLOB Utils

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

#include <freerdp/utils/memory.h>

#include <freerdp/utils/datablob.h>

/**
 * Allocate memory for data blob.
 * @param datablob datablob structure
 * @param length memory length
 */

void datablob_alloc(DATABLOB *datablob, int length)
{
	datablob->data = xmalloc(length);
	datablob->length = length;
}

/**
 * Free memory allocated for data blob.
 * @param datablob
 */

void datablob_free(DATABLOB *datablob)
{
	if (datablob->data)
		xfree(datablob->data);
	
	datablob->length = 0;
}
