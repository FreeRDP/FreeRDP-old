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
			GetBGR32(pixel->red, pixel->green, pixel->blue, color);
			break;
		case 24:
			GetBGR24(pixel->red, pixel->green, pixel->blue, color);
			break;
		case 16:
			GetRGB16(pixel->red, pixel->green, pixel->blue, color);
			break;
		case 15:
			GetBGR15(pixel->red, pixel->green, pixel->blue, color);
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
	char *src8;
	uint32 pixel;
	uint16 *src16;
	uint16 *dst16;
	uint32 *dst32;
	char *dstData;
	
	if (srcBpp == dstBpp)
	{
#ifdef USE_ALPHA
		int x, y;
		char *dstp;

		dstData = (char*) malloc(width * height * 4);
		memcpy(dstData, srcData, width * height * 4);
		
		dstp = dstData;
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width * 4; x += 4)
			{
				dstp += 3;
				*dstp = 0xFF;
				dstp++;
			}
		}
#else
		dstData = (char*) malloc(width * height * 4);
		memcpy(dstData, srcData, width * height * 4);
#endif
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
			pixel = RGB24(red, green, blue);
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
			GetBGR16(red, green, blue, pixel);
			pixel = BGR32(red, green, blue);
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
			GetRGB15(red, green, blue, pixel);
			pixel = BGR24(red, green, blue);
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
			GetRGB15(red, green, blue, pixel);
			pixel = RGB16(red, green, blue);
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
			pixel = RGB16(red, green, blue);
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
			pixel = RGB15(red, green, blue);
			*dst16 = pixel;
			dst16++;
		}
		return dstData;
	}

	return srcData;
}

char*
gdi_glyph_convert(int width, int height, char* data)
{
	int x, y;
	char *srcp;
	char *dstp;
	char *dstData;
	int scanline;
	
	/* 
	 * converts a 1-bit-per-pixel glyph to a one-byte-per-pixel glyph:
	 * this approach uses a little more memory, but provides faster
	 * means of accessing individual pixels in blitting operations
	 */

	scanline = (width + 7) / 8;
	dstData = (char*) malloc(width * height);
	memset(dstData, 0, width * height);
	dstp = dstData;
	
	for (y = 0; y < height; y++)
	{
		srcp = data + (y * scanline);
		
		for (x = 0; x < width; x++)
		{
			if ((*srcp & (0x80 >> (x % 8))) != 0)
				*dstp = 0xFF;
			dstp++;

			if (((x + 1) % 8 == 0) && x != 0)
				srcp++;
		}
	}
	
	return dstData;
}
