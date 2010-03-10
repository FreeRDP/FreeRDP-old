/*
   Copyright (c) 2009-2010 Jay Sorg
   Copyright (c) 2010 Vic Lee

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_win.h"
#include "wf_event.h"

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
  (((_blue & 0xff) >> 3) <<  0)

static int
get_pixel(uint8 * data, int x, int y, int width, int height, int bpp)
{
	int start;
	int shift;
	int red;
	int green;
	int blue;
	uint16 * s16;
	uint32 * s32;

	switch (bpp)
	{
		case  1:
			width = (width + 7) / 8;
			start = (y * width) + x / 8;
			shift = x % 8;
			return (data[start] & (0x80 >> shift)) != 0;
		case 8:
			return data[y * width + x];
		case 15:
		case 16:
			s16 = (uint16 *) data;
			return s16[y * width + x];
		case 24:
			data += y * width * 3;
			data += x * 3;
			red = data[0];
			green = data[1];
			blue = data[2];
			return MAKE24RGB(red, green, blue);
		case 32:
			s32 = (uint32 *) data;
			return s32[y * width + x];
		default:
			printf("unknonw in get_pixel\n");
			break;
	}
	return 0;
}

static void
set_pixel(uint8 * data, int x, int y, int width, int height, int bpp, int pixel)
{
	int start;
	int shift;
	int * d32;

	if (bpp == 1)
	{
		width = (width + 7) / 8;
		start = (y * width) + x / 8;
		shift = x % 8;
		if (pixel)
		{
			data[start] = data[start] | (0x80 >> shift);
		}
		else
		{
			data[start] = data[start] & ~(0x80 >> shift);
		}
	}
	else if (bpp == 32)
	{
		d32 = (int *) data;
		d32[y * width + x] = pixel;
	}
	else
	{
		printf("unknonw in set_pixel\n");
	}
}

int
wf_colour_convert(wfInfo * wfi, int in_colour, int in_bpp)
{
	int alpha;
	int red;
	int green;
	int blue;
	int rv;

	alpha = 0xff;
	red = 0;
	green = 0;
	blue = 0;
	rv = 0;
	switch (in_bpp)
	{
		case 32:
			SPLIT32BGR(alpha, red, green, blue, in_colour);
			break;
		case 24:
			SPLIT24BGR(red, green, blue, in_colour);
			break;
		case 16:
			SPLIT16RGB(red, green, blue, in_colour);
			break;
		case 15:
			SPLIT15RGB(red, green, blue, in_colour);
			break;
		case 8:
			in_colour &= 0xff;
			blue = *(wfi->colourmap + in_colour * 3);
			green = *(wfi->colourmap + in_colour * 3 + 1);
			red = *(wfi->colourmap + in_colour * 3 + 2);
			break;
		case 1:
			if (in_colour != 0)
			{
				red = 0xff;
				green = 0xff;
				blue = 0xff;
			}
			break;
		default:
			printf("wf_colour: bad in_bpp %d\n", in_bpp);
			break;
	}
	rv = RGB(red, green, blue);
	return rv;
}

uint8 *
wf_image_convert(wfInfo * wfi, int width, int height, int bpp,
	int reverse, uint8 * in_data, uint8 * out_data)
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
		for (indexy = 0; indexy < height; indexy++)
		{
			dst8 = out_data + ((reverse ? height - indexy - 1 : indexy) * bytes_per_line);
			for (indexx = 0; indexx < width; indexx++)
			{
				pixel = *src8++;
				memcpy(dst8, wfi->colourmap + pixel * 3, 3);
				dst8 += 3;
			}
		}
	}
	return out_data;
}

RD_HCOLOURMAP
wf_create_colourmap(wfInfo * wfi, RD_COLOURMAP * colours)
{
	uint8 * colourmap;
	uint8 * dst;
	int index;
	int count;

	colourmap = (uint8 *) malloc(3 * 256);
	memset(colourmap, 0, 3 * 256);
	count = colours->ncolours;
	if (count > 256)
	{
		count = 256;
	}
	dst = colourmap;
	for (index = 0; index < count; index++)
	{
		*dst++ = colours->colours[index].blue;
		*dst++ = colours->colours[index].green;
		*dst++ = colours->colours[index].red;
	}
	return (RD_HCOLOURMAP) colourmap;
}

int
wf_set_colourmap(wfInfo * wfi, RD_HCOLOURMAP map)
{
	if (wfi->colourmap != NULL)
	{
		free(wfi->colourmap);
	}
	wfi->colourmap = (uint8 *) map;
	return 0;
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
