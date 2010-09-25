/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   GDI Color Conversion Routines

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <freerdp/freerdp.h>

#include "gdi_color.h"
#include "gdi_window.h"
#include "libfreerdpgdi.h"

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

unsigned int
gdi_make_colorref(PIXEL *pixel)
{
	unsigned int colorref = 0;
	colorref = MAKE24RGB(pixel->red, pixel->green, pixel->blue);
	return colorref;
}

void
gdi_split_colorref(unsigned int colorref, PIXEL *pixel)
{
	pixel->alpha = 0;
	SPLIT24BGR(pixel->red, pixel->green, pixel->blue, colorref);
}

void
gdi_color_convert(PIXEL *pixel, int color, int bpp, HPALETTE palette)
{
	pixel->red = 0;
	pixel->green = 0;
	pixel->blue = 0;
	pixel->alpha = 0xFF;

	switch (bpp)
	{
		case 32:
			SPLIT32BGR(pixel->alpha, pixel->red, pixel->green, pixel->blue, color);
			break;
		case 24:
			SPLIT24BGR(pixel->red, pixel->green, pixel->blue, color);
			break;
		case 16:
			SPLIT16RGB(pixel->red, pixel->green, pixel->blue, color);
			break;
		case 15:
			SPLIT15RGB(pixel->red, pixel->green, pixel->blue, color);
			break;
		case 8:
			color &= 0xFF;
			pixel->red = palette->logicalPalette->entries[color].red;
			pixel->green = palette->logicalPalette->entries[color].green;
			pixel->blue = palette->logicalPalette->entries[color].blue;
			break;
		default:
			break;
	}
}

char*
gdi_image_convert(char* srcData, int width, int height, int srcBpp, int dstBpp, HPALETTE palette)
{
	int red;
	int green;
	int blue;
	int index;
	int pixel;
	char *src8;
	unsigned short *src16;
	unsigned short *dst16;
	unsigned int *dst32;
	char *dstData;

	if (srcBpp == dstBpp)
	{
		dstData = (char*) malloc(width * height * 4);
		memcpy(dstData, srcData, width * height * 4);
		return dstData;
	}
	if ((srcBpp == 24) && (dstBpp == 32))
	{
		dstData = (char*) malloc(width * height * 4);
		src8 = srcData;
		dst32 = (uint32 *) dstData;
		for (index = width * height; index > 0; index--)
		{
			blue = *(src8++);
			green = *(src8++);
			red = *(src8++);
			pixel = MAKE24RGB(red, green, blue);
			*dst32 = pixel;
			dst32++;
		}
		return dstData;
	}
	else if ((srcBpp == 16) && (dstBpp == 32))
	{
		dstData = (char*) malloc(width * height * 4);
		src16 = (uint16 *) srcData;
		dst32 = (uint32 *) dstData;
		for (index = width * height; index > 0; index--)
		{
			pixel = *src16;
			src16++;
			SPLIT16RGB(red, green, blue, pixel);
			pixel = MAKE24RGB(red, green, blue);
			*dst32 = pixel;
			dst32++;
		}
		return dstData;
	}
	else if ((srcBpp == 15) && (dstBpp == 32))
	{
		dstData = (char*) malloc(width * height * 4);
		src16 = (uint16 *) srcData;
		dst32 = (uint32 *) dstData;
		for (index = width * height; index > 0; index--)
		{
			pixel = *src16;
			src16++;
			SPLIT15RGB(red, green, blue, pixel);
			pixel = MAKE24RGB(red, green, blue);
			*dst32 = pixel;
			dst32++;
		}
		return dstData;
	}
	else if ((srcBpp == 15) && (dstBpp == 16))
	{
		dstData = (char*) malloc(width * height * 2);
		src16 = (uint16 *) srcData;
		dst16 = (uint16 *) dstData;
		for (index = width * height; index > 0; index--)
		{
			pixel = *src16;
			src16++;
			SPLIT15RGB(red, green, blue, pixel);
			pixel = MAKE16RGB(red, green, blue);
			*dst16 = pixel;
			dst16++;
		}
		return dstData;
	}
	else if ((srcBpp == 8) && (dstBpp == 16))
	{
		dstData = (char *) malloc(width * height * 2);
		src8 = srcData;
		dst16 = (uint16 *) dstData;
		for (index = width * height; index > 0; index--)
		{
			pixel = *src8;
			src8++;
			red = palette->logicalPalette->entries[pixel].red;
			green = palette->logicalPalette->entries[pixel].green;
			blue = palette->logicalPalette->entries[pixel].blue;
			pixel = MAKE16RGB(red, green, blue);
			*dst16 = pixel;
			dst16++;
		}
		return dstData;
	}
	else if ((srcBpp == 8) && (dstBpp == 15))
	{
		dstData = (char*) malloc(width * height * 2);
		src8 = srcData;
		dst16 = (uint16 *) dstData;
		for (index = width * height; index > 0; index--)
		{
			pixel = *src8;
			src8++;
			red = palette->logicalPalette->entries[pixel].red;
			green = palette->logicalPalette->entries[pixel].green;
			blue = palette->logicalPalette->entries[pixel].blue;
			pixel = MAKE15RGB(red, green, blue);
			*dst16 = pixel;
			dst16++;
		}
		return dstData;
	}

	return srcData;
}
