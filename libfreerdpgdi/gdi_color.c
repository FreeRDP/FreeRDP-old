/*
   FreeRDP: A Remote Desktop Protocol client.
   GDI Color Conversion Routines

   Copyright 2010 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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
		if (dstBpp == 32)
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
		}
		else if (dstBpp == 16)
		{
			dstData = (char*) malloc(width * height * 2);
#ifdef GDI_SWAP_16BPP
			src16 = (uint16*) srcData;
			dst16 = (uint16*) dstData;
			for (index = width * height; index > 0; index--)
			{
				*dst16 = (*src16 >> 8) | (*src16 << 8);
				src16++;
				dst16++;
			}
#else
			memcpy(dstData, srcData, width * height * 2);
#endif
		}
		else
		{
			printf("OMG HAX BPP:%d\n", dstBpp);
			dstData = (char*) malloc(width * height * 4);
			memcpy(dstData, srcData, width * height * 4);
		}
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
