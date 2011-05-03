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

#define SPLIT32BGR(_alpha, _red, _green, _blue, _pixel) \
  _red = _pixel & 0xff; \
  _green = (_pixel & 0xff00) >> 8; \
  _blue = (_pixel & 0xff0000) >> 16; \
  _alpha = (_pixel & 0xff000000) >> 24;

#define SPLIT24BGR(_red, _green, _blue, _pixel) \
  _red = _pixel & 0xff; \
  _green = (_pixel & 0xff00) >> 8; \
  _blue = (_pixel & 0xff0000) >> 16;

#define SPLIT24RGB(_red, _green, _blue, _pixel) \
  _blue  = _pixel & 0xff; \
  _green = (_pixel & 0xff00) >> 8; \
  _red   = (_pixel & 0xff0000) >> 16;

#define SPLIT16RGB(_red, _green, _blue, _pixel) \
  _red = ((_pixel >> 8) & 0xf8) | ((_pixel >> 13) & 0x7); \
  _green = ((_pixel >> 3) & 0xfc) | ((_pixel >> 9) & 0x3); \
  _blue = ((_pixel << 3) & 0xf8) | ((_pixel >> 2) & 0x7);

#define SPLIT15RGB(_red, _green, _blue, _pixel) \
  _red = ((_pixel >> 7) & 0xf8) | ((_pixel >> 12) & 0x7); \
  _green = ((_pixel >> 2) & 0xf8) | ((_pixel >> 8) & 0x7); \
  _blue = ((_pixel << 3) & 0xf8) | ((_pixel >> 2) & 0x7);

#define MAKE24RGB(_red, _green, _blue) \
  (_red << 16) | (_green << 8) | _blue;

#define MAKE15RGB(_red, _green, _blue) \
  (((_red & 0xff) >> 3) << 10) | \
  (((_green & 0xff) >> 3) <<  5) | \
  (((_blue & 0xff) >> 3) <<  0)

#define MAKE16RGB(_red, _green, _blue) \
  (((_red & 0xff) >> 3) << 11) | \
  (((_green & 0xff) >> 2) <<  5) | \
  (((_blue & 0xff) >> 3) <<  0)s

uint8 *
wf_image_convert(wfInfo * wfi, int width, int height, int bpp, int reverse, uint8 * in_data, uint8 * out_data)
{
	int red;
	int green;
	int blue;
	int indexx;
	int indexy;
	int pixel;
	int bytes_per_line;
	uint8 * src8;
	uint16 * src16;
	uint8 * dst8;

	if (out_data == NULL)
	{
		out_data = (uint8 *) malloc(width * height * 3);
	}
	bytes_per_line = width * 3;
	bytes_per_line = ((bytes_per_line + 3) / 4) * 4;
	if (bpp == 32)
	{
		src8 = in_data;
		for (indexy = 0; indexy < height; indexy++)
		{
			dst8 = out_data + ((reverse ? height - indexy - 1 : indexy) * bytes_per_line);
			for (indexx = 0; indexx < width; indexx++)
			{
				*dst8++ = *src8++;
				*dst8++ = *src8++;
				*dst8++ = *src8++;
				src8++;
			}
		}
	}
	else if (bpp == 24)
	{
		src8 = in_data;
		for (indexy = 0; indexy < height; indexy++)
		{
			dst8 = out_data + ((reverse ? height - indexy - 1 : indexy) * bytes_per_line);
			memcpy(dst8, src8, width * 3);
			src8 += width * 3;
		}
	}
	else if (bpp == 16)
	{
		src16 = (uint16 *) in_data;
		for (indexy = 0; indexy < height; indexy++)
		{
			dst8 = out_data + ((reverse ? height - indexy - 1 : indexy) * bytes_per_line);
			for (indexx = 0; indexx < width; indexx++)
			{
				pixel = *src16;
				src16++;
				SPLIT16RGB(red, green, blue, pixel);
				pixel = RGB(red, green, blue);
				*dst8++ = blue;
				*dst8++ = green;
				*dst8++ = red;
			}
		}
	}
	else if (bpp == 15)
	{
		src16 = (uint16 *) in_data;
		for (indexy = 0; indexy < height; indexy++)
		{
			dst8 = out_data + ((reverse ? height - indexy - 1 : indexy) * bytes_per_line);
			for (indexx = 0; indexx < width; indexx++)
			{
				pixel = *src16;
				src16++;
				SPLIT15RGB(red, green, blue, pixel);
				pixel = RGB(red, green, blue);
				*dst8++ = blue;
				*dst8++ = green;
				*dst8++ = red;
			}
		}
	}
	else if (bpp == 8)
	{
		src8 = in_data;
		uint8* palette = (uint8*) wfi->palette->entries;	
		for (indexy = 0; indexy < height; indexy++)
		{
			dst8 = out_data + ((reverse ? height - indexy - 1 : indexy) * bytes_per_line);
			for (indexx = 0; indexx < width; indexx++)
			{
				pixel = *src8++;
				memcpy(dst8, palette + pixel * 3, 3);
				dst8 += 3;
			}
		}
	}
	return out_data;
}

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
