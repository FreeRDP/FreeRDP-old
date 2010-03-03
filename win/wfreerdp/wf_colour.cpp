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

static int
wf_colour(wfInfo * wfi, int in_colour, int in_bpp)
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
			blue = *(wfi->colourmap + in_colour * 4);
			green = *(wfi->colourmap + in_colour * 4 + 1);
			red = *(wfi->colourmap + in_colour * 4 + 2);
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

int
wf_colour_convert(wfInfo * wfi, rdpSet * settings, int colour)
{
	return wf_colour(wfi, colour, settings->server_depth);
}

uint8 *
wf_image_convert(wfInfo * wfi, rdpSet * settings, int width, int height,
	uint8 * in_data)
{
	int red;
	int green;
	int blue;
	int indexx;
	int indexy;
	int pixel;
	uint8 * out_data;
	uint8 * src8;
	uint16 * src16;
	uint8 * dst8;

	if (settings->server_depth == 24)
	{
		out_data = (uint8 *) malloc(width * height * 4);
		src8 = in_data;
		dst8 = out_data;
		for (indexy = height - 1; indexy >= 0; indexy--)
		{
			dst8 = out_data + (indexy * width * 4);
			for (indexx = 0; indexx < width; indexx++)
			{
				*dst8++ = *src8++;
				*dst8++ = *src8++;
				*dst8++ = *src8++;
				*dst8++ = 0;
			}
		}
		return out_data;
	}
	else if (settings->server_depth == 16)
	{
		out_data = (uint8 *) malloc(width * height * 4);
		src16 = (uint16 *) in_data;
		for (indexy = height - 1; indexy >= 0; indexy--)
		{
			dst8 = out_data + (indexy * width * 4);
			for (indexx = 0; indexx < width; indexx++)
			{
				pixel = *src16;
				src16++;
				SPLIT16RGB(red, green, blue, pixel);
				pixel = RGB(red, green, blue);
				*dst8++ = blue;
				*dst8++ = green;
				*dst8++ = red;
				*dst8++ = 0;
			}
		}
		return out_data;
	}
	else if (settings->server_depth == 15)
	{
		out_data = (uint8 *) malloc(width * height * 4);
		src16 = (uint16 *) in_data;
		for (indexy = height - 1; indexy >= 0; indexy--)
		{
			dst8 = out_data + (indexy * width * 4);
			for (indexx = 0; indexx < width; indexx++)
			{
				pixel = *src16;
				src16++;
				SPLIT15RGB(red, green, blue, pixel);
				pixel = RGB(red, green, blue);
				*dst8++ = blue;
				*dst8++ = green;
				*dst8++ = red;
				*dst8++ = 0;
			}
		}
		return out_data;
	}
	else if (settings->server_depth == 8)
	{
		out_data = (uint8 *) malloc(width * height * 4);
		src8 = in_data;
		for (indexy = height - 1; indexy >= 0; indexy--)
		{
			dst8 = out_data + (indexy * width * 4);
			for (indexx = 0; indexx < width; indexx++)
			{
				pixel = *src8++;
				memcpy(dst8, wfi->colourmap + pixel * 4, 4);
				dst8 += 4;
			}
		}
		return out_data;
	}
	return in_data;
}

RD_HCOLOURMAP
wf_create_colourmap(wfInfo * wfi, rdpSet * settings, RD_COLOURMAP * colours)
{
	uint8 * colourmap;
	uint8 * dst;
	int index;
	int count;

	colourmap = (uint8 *) malloc(4 * 256);
	memset(colourmap, 0, 4 * 256);
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
		*dst++ = 0;
	}
	return (RD_HCOLOURMAP) colourmap;
}

int
wf_set_colourmap(wfInfo * wfi, rdpSet * settings, RD_HCOLOURMAP map)
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
	int indexx;
	int indexy;

	cdata = (uint8 *) malloc(width * height);
	src = data;
	for (indexy = height - 1; indexy >= 0; indexy--)
	{
		dst = cdata + indexy * width;
		for (indexx = 0; indexx < width; indexx++)
		{
			*dst++ = ((*src & (0x80 >> (indexx % 8))) ? 1 : 0);
			if ((indexx % 8) == 7 || indexx == width - 1)
			{
				src++;
			}
		}
	}
	return cdata;
}

uint8 *
wf_glyph_generate(wfInfo * wfi, int width, int height, uint8 * glyph)
{
	uint8 * src;
	uint8 * data;
	uint8 * dst;
	int index;

	src = (uint8 *) glyph;
	data = (uint8 *) malloc(width * height * 4);
	dst = data;
	for (index = width * height; index > 0; index--)
	{
		if (*src)
		{
			*dst++ = ((wfi->fgcolour >> 16) & 0xff);
			*dst++ = ((wfi->fgcolour >> 8) & 0xff);
			*dst++ = (wfi->fgcolour & 0xff);
			*dst++ = 0;
		}
		else
		{
			*dst++ = ((wfi->bgcolour >> 16) & 0xff);
			*dst++ = ((wfi->bgcolour >> 8) & 0xff);
			*dst++ = (wfi->bgcolour & 0xff);
			*dst++ = 0;
		}
		src++;
	}
	return data;
}
