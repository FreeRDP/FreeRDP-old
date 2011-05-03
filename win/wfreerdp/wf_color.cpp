/*
   FreeRDP: A Remote Desktop Protocol client.

   Copyright (c) 2009-2011 Jay Sorg
   Copyright (c) 2010-2011 Vic Lee

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_types.h"
#include "wf_win.h"

uint8 *
wf_glyph_convert(wfInfo * wfi, int width, int height, uint8 * data)
{
	uint8 * cdata;
	uint8 * src;
	uint8 * dst;
	int src_bytes_per_row;
	int dst_bytes_per_row;
	int indexx;
	int indexy;

	src_bytes_per_row = (width + 7) / 8;
	dst_bytes_per_row = src_bytes_per_row + (src_bytes_per_row % 2);
	cdata = (uint8 *) malloc(dst_bytes_per_row * height);
	src = data;
	for (indexy = 0; indexy < height; indexy++)
	{
		dst = cdata + indexy * dst_bytes_per_row;
		for (indexx = 0; indexx < dst_bytes_per_row; indexx++)
		{
			if (indexx < src_bytes_per_row)
			{
				*dst++ = *src++;
			}
			else
			{
				*dst++ = 0;
			}
		}
	}
	return cdata;
}

uint8 *
wf_cursor_mask_convert(wfInfo * wfi, int width, int height, uint8 * data)
{
	int indexy;
	uint8 * cdata;
	uint8 * src;
	uint8 * dst;

	cdata = (uint8 *) malloc(width * height / 8);
	src = data;
	for (indexy = height - 1; indexy >= 0; indexy--)
	{
		dst = cdata + (indexy * width / 8);
		memcpy(dst, src, width / 8);
		src += width / 8;
	}
	return cdata;
}
