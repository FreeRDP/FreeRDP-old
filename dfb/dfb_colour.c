/*
   Copyright (c) 2009-2010 Jay Sorg

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

/*
  Valid colour conversions
    8 -> 32   8 -> 24   8 -> 16   8 -> 15
    15 -> 32  15 -> 24  15 -> 16  15 -> 15
    16 -> 32  16 -> 24  16 -> 16
    24 -> 32  24 -> 24
    32 -> 32
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dfb_win.h"
#include "dfb_event.h"
#include "dfb_colour.h"

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

#define MAKE32RGB(_alpha, _red, _green, _blue) \
  (_alpha << 24) | (_red << 16) | (_green << 8) | _blue;

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

void
dfb_colour_convert(dfbInfo * dfbi, int in_colour, PIXEL * pixel, int in_bpp, int out_bpp)
{
	pixel->red = 0;
	pixel->green = 0;
	pixel->blue = 0;
	pixel->alpha = 0xFF;
	
	printf("in_colour:%X in_bpp:%d out_bpp:%d\n", in_colour,in_bpp,out_bpp);
	
	switch (in_bpp)
	{
		case 32:
			SPLIT32BGR(pixel->alpha, pixel->red, pixel->green, pixel->blue, in_colour);
			break;
		case 24:
			SPLIT24BGR(pixel->red, pixel->green, pixel->blue, in_colour);
			break;
		case 16:
			SPLIT16RGB(pixel->red, pixel->green, pixel->blue, in_colour);
			break;
		case 15:
			SPLIT15RGB(pixel->red, pixel->green, pixel->blue, in_colour);
			break;
		case 8:
			in_colour &= 0xFF;
			SPLIT24RGB(pixel->red, pixel->green, pixel->blue, dfbi->colourmap[in_colour]);
			break;
		default:
			printf("dfb_colour: bad in_bpp %d\n", in_bpp);
			break;
	}
}

uint8 *
dfb_image_convert(dfbInfo * dfbi, rdpSet * settings, int width, int height, uint8 * in_data)
{
	int red;
	int green;
	int blue;
	int index;
	int pixel;
	uint8 * out_data;
	uint8 * src8;
	uint16 * src16;
	uint16 * dst16;
	uint32 * dst32;

	if ((settings->server_depth == 24) && (dfbi->bpp == 32))
	{
		out_data = (uint8 *) malloc(width * height * 4);
		src8 = in_data;
		dst32 = (uint32 *) out_data;
		for (index = width * height; index > 0; index--)
		{
			blue = *(src8++);
			green = *(src8++);
			red = *(src8++);
			pixel = MAKE24RGB(red, green, blue);
			*dst32 = pixel;
			dst32++;
		}
		return out_data;
	}
	else if ((settings->server_depth == 16) && (dfbi->bpp == 32))
	{
		out_data = (uint8 *) malloc(width * height * 4);
		src16 = (uint16 *) in_data;
		dst32 = (uint32 *) out_data;
		for (index = width * height; index > 0; index--)
		{
			pixel = *src16;
			src16++;
			SPLIT16RGB(red, green, blue, pixel);
			pixel = MAKE24RGB(red, green, blue);
			*dst32 = pixel;
			dst32++;
		}
		return out_data;
	}
	else if ((settings->server_depth == 15) && (dfbi->bpp == 32))
	{
		out_data = (uint8 *) malloc(width * height * 4);
		src16 = (uint16 *) in_data;
		dst32 = (uint32 *) out_data;
		for (index = width * height; index > 0; index--)
		{
			pixel = *src16;
			src16++;
			SPLIT15RGB(red, green, blue, pixel);
			pixel = MAKE24RGB(red, green, blue);
			*dst32 = pixel;
			dst32++;
		}
		return out_data;
	}
	else if ((settings->server_depth == 8) && (dfbi->bpp == 32))
	{
		out_data = (uint8 *) malloc(width * height * 4);
		src8 = in_data;
		dst32 = (uint32 *) out_data;
		for (index = width * height; index > 0; index--)
		{
			pixel = *src8;
			src8++;
			pixel = dfbi->colourmap[pixel];
			*dst32 = pixel;
			dst32++;
		}
		return out_data;
	}
	else if ((settings->server_depth == 15) && (dfbi->bpp == 16))
	{
		out_data = (uint8 *) malloc(width * height * 2);
		src16 = (uint16 *) in_data;
		dst16 = (uint16 *) out_data;
		for (index = width * height; index > 0; index--)
		{
			pixel = *src16;
			src16++;
			SPLIT15RGB(red, green, blue, pixel);
			pixel = MAKE16RGB(red, green, blue);
			*dst16 = pixel;
			dst16++;
		}
		return out_data;
	}
	else if ((settings->server_depth == 8) && (dfbi->bpp == 16))
	{
		out_data = (uint8 *) malloc(width * height * 2);
		src8 = in_data;
		dst16 = (uint16 *) out_data;
		for (index = width * height; index > 0; index--)
		{
			pixel = *src8;
			src8++;
			pixel = dfbi->colourmap[pixel];
			SPLIT24RGB(red, green, blue, pixel);
			pixel = MAKE16RGB(red, green, blue);
			*dst16 = pixel;
			dst16++;
		}
		return out_data;
	}
	else if ((settings->server_depth == 8) && (dfbi->bpp == 15))
	{
		out_data = (uint8 *) malloc(width * height * 2);
		src8 = in_data;
		dst16 = (uint16 *) out_data;
		for (index = width * height; index > 0; index--)
		{
			pixel = *src8;
			src8++;
			pixel = dfbi->colourmap[pixel];
			SPLIT24RGB(red, green, blue, pixel);
			pixel = MAKE15RGB(red, green, blue);
			*dst16 = pixel;
			dst16++;
		}
		return out_data;
	}
	return in_data;
}

RD_HCOLOURMAP
dfb_create_colourmap(dfbInfo * dfbi, rdpSet * settings, RD_COLOURMAP * colours)
{
	int index;
	int red;
	int green;
	int blue;
	int count;
	int * colourmap;
	
	colourmap = (int *) malloc(sizeof(int) * 256);
	memset(colourmap, 0, sizeof(int) * 256);
	count = colours->ncolours;
	
	if (count > 256)
		count = 256;
	
	for (index = count - 1; index >= 0; index--)
	{
		red = colours->colours[index].red;
		green = colours->colours[index].green;
		blue = colours->colours[index].blue;
		colourmap[index] = MAKE24RGB(red, green, blue);
	}
	
	return (RD_HCOLOURMAP) colourmap;
}

int
dfb_set_colourmap(dfbInfo * dfbi, rdpSet * settings, RD_HCOLOURMAP map)
{
	if (dfbi->colourmap != NULL)
		free(dfbi->colourmap);

	dfbi->colourmap = (int *) map;
	
	return 0;
}

/* create mono cursor */
int
dfb_cursor_convert_mono(dfbInfo * dfbi, uint8 * src_data, uint8 * msk_data,
	uint8 * xormask, uint8 * andmask, int width, int height, int bpp)
{

	return 0;
}

/* create 32 bpp cursor */
int
dfb_cursor_convert_alpha(dfbInfo * dfbi, uint8 * alpha_data,
	uint8 * xormask, uint8 * andmask, int width, int height, int bpp)
{

	return 0;
}
