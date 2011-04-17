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
#include "gdi.h"

#include "gdi_brush.h"

int gdi_get_pixel(uint8 * data, int x, int y, int width, int height, int bpp)
{
	int start;
	int shift;
	uint16 *src16;
	uint32 *src32;
	int red, green, blue;

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
			src16 = (uint16*) data;
			return src16[y * width + x];
		case 24:
			data += y * width * 3;
			data += x * 3;
			red = data[0];
			green = data[1];
			blue = data[2];
			return RGB24(red, green, blue);
		case 32:
			src32 = (uint32*) data;
			return src32[y * width + x];
		default:
			break;
	}

	return 0;
}

void gdi_set_pixel(uint8* data, int x, int y, int width, int height, int bpp, int pixel)
{
	int start;
	int shift;
	int *dst32;

	if (bpp == 1)
	{
		width = (width + 7) / 8;
		start = (y * width) + x / 8;
		shift = x % 8;
		if (pixel)
			data[start] = data[start] | (0x80 >> shift);
		else
			data[start] = data[start] & ~(0x80 >> shift);
	}
	else if (bpp == 32)
	{
		dst32 = (int*) data;
		dst32[y * width + x] = pixel;
	}
}

int gdi_color(int srcColor, int srcBpp, int dstBpp, HPALETTE palette)
{
	uint8 red = 0;
	uint8 green = 0;
	uint8 blue = 0;
	uint8 alpha = 0xFF;
	int dstColor = 0;

	switch (srcBpp)
	{
		case 32:
			GetBGR32(red, green, blue, srcColor);
			break;
		case 24:
			GetBGR24(red, green, blue, srcColor);
			break;
		case 16:
			GetRGB16(red, green, blue, srcColor);
			break;
		case 15:
			GetBGR15(red, green, blue, srcColor);
			break;
		case 8:
			srcColor &= 0xFF;
			red = palette->entries[srcColor].red;
			green = palette->entries[srcColor].green;
			blue = palette->entries[srcColor].blue;
			break;
		case 1:
			if (srcColor != 0)
			{
				red = 0xFF;
				green = 0xFF;
				blue = 0xFF;
			}
			break;
		default:
			break;
	}
	switch (dstBpp)
	{
		case 32:
			dstColor = ARGB32(alpha, red, green, blue);
			break;
		case 24:
			dstColor = RGB24(red, green, blue);
			break;
		case 16:
			dstColor = RGB16(red, green, blue);
			break;
		case 15:
			dstColor = RGB15(red, green, blue);
			break;
		case 1:
			if ((red != 0) || (green != 0) || (blue != 0))
				dstColor = 1;
			break;
		default:
			break;
	}

	return dstColor;
}

void gdi_color_convert(PIXEL *pixel, int color, int bpp, HPALETTE palette)
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
			pixel->red = palette->entries[color].red;
			pixel->green = palette->entries[color].green;
			pixel->blue = palette->entries[color].blue;
			break;
		default:
			break;
	}
}

uint8* gdi_image_convert_8bpp(uint8* srcData, int width, int height, int srcBpp, int dstBpp, HPALETTE palette)
{
	int i;
	uint8 red;
	uint8 green;
	uint8 blue;
	uint32 pixel;
	uint8 *src8;
	uint16 *dst16;
	uint32 *dst32;
	uint8 *dstData;

	if (dstBpp == 8)
	{
		dstData = (uint8*) malloc(width * height);
		memcpy(dstData, srcData, width * height);
		return dstData;
	}
	else if (dstBpp == 16)
	{
		dstData = (uint8*) malloc(width * height * 2);
		dst16 = (uint16 *) dstData;
		for (i = width * height; i > 0; i--)
		{
			pixel = *srcData;
			srcData++;
			red = palette->entries[pixel].red;
			green = palette->entries[pixel].green;
			blue = palette->entries[pixel].blue;
			RGB_888_565(red, green, blue);
			pixel = RGB16(red, green, blue);
			*dst16 = pixel;
			dst16++;
		}
		return dstData;
	}
	else if (dstBpp == 15)
	{
		dstData = (uint8*) malloc(width * height * 2);
		dst16 = (uint16 *) dstData;
		for (i = width * height; i > 0; i--)
		{
			pixel = *srcData;
			srcData++;
			red = palette->entries[pixel].red;
			green = palette->entries[pixel].green;
			blue = palette->entries[pixel].blue;
			RGB_888_555(red, green, blue);
			pixel = RGB15(red, green, blue);
			*dst16 = pixel;
			dst16++;
		}
		return dstData;
	}
	else if (dstBpp == 32)
	{
		dstData = (uint8*) malloc(width * height * 4);
		src8 = (uint8*) srcData;
		dst32 = (uint32*) dstData;
		for (i = width * height; i > 0; i--)
		{
			pixel = *src8;
			src8++;
			red = palette->entries[pixel].red;
			green = palette->entries[pixel].green;
			blue = palette->entries[pixel].blue;
			pixel = BGR32(red, green, blue);
			*dst32 = pixel;
			dst32++;
		}
		return dstData;
	}

	return srcData;
}

uint8* gdi_image_convert_15bpp(uint8* srcData, int width, int height, int srcBpp, int dstBpp, HPALETTE palette)
{
	int i;
	uint8 red;
	uint8 green;
	uint8 blue;
	uint32 pixel;
	uint16 *src16;
	uint16 *dst16;
	uint32 *dst32;
	uint8 *dstData;

	if (dstBpp == 15 || dstBpp == 16)
	{
#ifdef GDI_SWAP_16BPP
		src16 = (uint16*) srcData;
		dst16 = (uint16*) dstData;
		dstData = (uint8*) malloc(width * height * 2);
		for (i = width * height; i > 0; i--)
		{
			*dst16 = (*src16 >> 8) | (*src16 << 8);
			src16++;
			dst16++;
		}
#else
		dstData = (uint8*) malloc(width * height * 2);
		memcpy(dstData, srcData, width * height * 2);
#endif
		return dstData;
	}
	else if (dstBpp == 32)
	{
		dstData = (uint8*) malloc(width * height * 4);
		src16 = (uint16 *) srcData;
		dst32 = (uint32 *) dstData;
		for (i = width * height; i > 0; i--)
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
	else if (dstBpp == 16)
	{
		dstData = (uint8*) malloc(width * height * 2);
		src16 = (uint16 *) srcData;
		dst16 = (uint16 *) dstData;
		for (i = width * height; i > 0; i--)
		{
			pixel = *src16;
			src16++;
			GetRGB15(red, green, blue, pixel);
			RGB_555_565(red, green, blue);
			pixel = RGB16(red, green, blue);
			*dst16 = pixel;
			dst16++;
		}
		return dstData;
	}

	return srcData;
}

uint8* gdi_image_convert_16bpp(uint8* srcData, int width, int height, int srcBpp, int dstBpp, HPALETTE palette)
{
	uint8 *dstData;

	if (srcBpp == 15)
		return gdi_image_convert_15bpp(srcData, width, height, srcBpp, dstBpp, palette);

	if (dstBpp == 16)
	{
#ifdef GDI_SWAP_16BPP
		int i;
		uint16* src16 = (uint16*) srcData;
		uint16* dst16 = (uint16*) dstData;
		dstData = (uint8*) malloc(width * height * 2);
		for (i = width * height; i > 0; i--)
		{
			*dst16 = (*src16 >> 8) | (*src16 << 8);
			src16++;
			dst16++;
		}
#else
		dstData = (uint8*) malloc(width * height * 2);
		memcpy(dstData, srcData, width * height * 2);
#endif
		return dstData;
	}

	return srcData;
}

uint8* gdi_image_convert_24bpp(uint8* srcData, int width, int height, int srcBpp, int dstBpp, HPALETTE palette)
{
	uint8 red;
	uint8 green;
	uint8 blue;
	int index;
	uint32 pixel;
	uint32 *dst32;
	uint8 *dstData;

	if (dstBpp == 24)
	{
		dstData = (uint8*) malloc(width * height * 3);
		memcpy(dstData, srcData, width * height * 3);
		return dstData;
	}
	else if (dstBpp == 32)
	{
		dstData = (uint8*) malloc(width * height * 4);
		dst32 = (uint32 *) dstData;
		for (index = width * height; index > 0; index--)
		{
			red = *(srcData++);
			green = *(srcData++);
			blue = *(srcData++);
			pixel = BGR24(red, green, blue);
			*dst32 = pixel;
			dst32++;
		}
		return dstData;
	}

	return srcData;
}

uint8* gdi_image_convert_32bpp(uint8* srcData, int width, int height, int srcBpp, int dstBpp, HPALETTE palette)
{
	uint8 *dstData;

	if (dstBpp == 32)
	{
#ifdef USE_ALPHA
		int x, y;
		uint8 *dstp;

		dstData = (uint8*) malloc(width * height * 4);
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
		dstData = (uint8*) malloc(width * height * 4);
		memcpy(dstData, srcData, width * height * 4);
#endif
		return dstData;
	}

	return srcData;
}

p_gdi_image_convert gdi_image_convert_[5] =
{
	NULL,
	gdi_image_convert_8bpp,
	gdi_image_convert_16bpp,
	gdi_image_convert_24bpp,
	gdi_image_convert_32bpp
};

uint8* gdi_image_convert(uint8* srcData, int width, int height, int srcBpp, int dstBpp, HPALETTE palette)
{
	p_gdi_image_convert _p_gdi_image_convert = gdi_image_convert_[IBPP(srcBpp)];

	if (_p_gdi_image_convert != NULL)
		return _p_gdi_image_convert(srcData, width, height, srcBpp, dstBpp, palette);
	else
		return 0;
}

uint8*
gdi_glyph_convert(int width, int height, uint8* data)
{
	int x, y;
	uint8 *srcp;
	uint8 *dstp;
	uint8 *dstData;
	int scanline;

	/*
	 * converts a 1-bit-per-pixel glyph to a one-byte-per-pixel glyph:
	 * this approach uses a little more memory, but provides faster
	 * means of accessing individual pixels in blitting operations
	 */

	scanline = (width + 7) / 8;
	dstData = (uint8*) malloc(width * height);
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


uint8* gdi_mono_image_convert(uint8* srcData, int width, int height, int srcBpp, int dstBpp, int bgcolor, int fgcolor, HPALETTE palette)
{
	int index;
	uint16* dst16;
	uint32* dst32;
	uint8* dstData;
	uint8 bitMask;
	int bitIndex;
	uint8 redBg, greenBg, blueBg;
	uint8 redFg, greenFg, blueFg;

	if(dstBpp == 16)
	{
		if(srcBpp == 8)
		{
			/* convert palette colors to 16-bit colors */
			bgcolor &= 0xFF;
			redBg = palette->entries[bgcolor].red;
			greenBg = palette->entries[bgcolor].green;
			blueBg = palette->entries[bgcolor].blue;
			RGB_888_565(redBg, greenBg, blueBg);
			bgcolor = RGB16(redBg, greenBg, blueBg);

			fgcolor &= 0xFF;
			redFg = palette->entries[fgcolor].red;
			greenFg = palette->entries[fgcolor].green;
			blueFg = palette->entries[fgcolor].blue;
			RGB_888_565(redFg, greenFg, blueFg);
			fgcolor = RGB16(redFg, greenFg, blueFg);
		}
		if(srcBpp == 15)
		{
			/* convert 15-bit colors to 16-bit colors */
			RGB15_RGB16(redBg, greenBg, blueBg, bgcolor);
			RGB15_RGB16(redFg, greenFg, blueFg, fgcolor);
		}

		dstData = (uint8*) malloc(width * height * 2);
		dst16 = (uint16*) dstData;
		for(index = height; index > 0; index--)
		{
			/* each bit encodes a pixel */
			bitMask = *srcData;
			for(bitIndex = 7; bitIndex >= 0; bitIndex--)
			{
				if((bitMask >> bitIndex) & 0x01)
				{
					*dst16 = bgcolor;
				}
				else
				{
					*dst16 = fgcolor;
				}
				dst16++;
			}
			srcData++;
		}
		return dstData;
	}
	else if(dstBpp == 32)
	{
		GetRGB(redBg, greenBg, blueBg, bgcolor);
		GetRGB(redFg, greenFg, blueFg, fgcolor);

		dstData = (uint8*) malloc(width * height * 4);
		dst32 = (uint32*) dstData;
		for(index = height; index > 0; index--)
		{
			/* each bit encodes a pixel */
			bitMask = *srcData;
			for(bitIndex = 7; bitIndex >= 0; bitIndex--)
			{
				if((bitMask >> bitIndex) & 0x01)
				{
					*dst32 = BGR32(redBg, greenBg, blueBg);
				}
				else
				{
					*dst32 = BGR32(redFg, greenFg, blueFg);
				}
				dst32++;
			}
			srcData++;
		}
		return dstData;
	}
	else
		DEBUG_GDI("conversion to %d bpp not implemented", dstBpp);

	return srcData;
}

/* create mono cursor */
int gdi_mono_cursor_convert(uint8* srcData, uint8* maskData, uint8* xorMask, uint8* andMask, int width, int height, int bpp, HPALETTE palette)
{
	int i,j,jj;
	uint32 xpixel;
	uint32 apixel;

	for (j = 0; j < height; j++)
	{
		jj = (bpp == 1) ? j : (height - 1) - j;
		for (i = 0; i < width; i++)
		{
			xpixel = gdi_get_pixel(xorMask, i, jj, width, height, bpp);
			xpixel = gdi_color(xpixel, bpp, 24, palette);
			apixel = gdi_get_pixel(andMask, i, jj, width, height, 1);

			if ((xpixel == 0xffffff) && (apixel != 0))
			{
				/* use pattern (not solid black) for xor area */
				xpixel = (i & 1) == (j & 1);
				apixel = 1;
			}
			else
			{
				xpixel = xpixel != 0;
				apixel = apixel == 0;
			}

			gdi_set_pixel(srcData, i, j, width, height, 1, xpixel);
			gdi_set_pixel(maskData, i, j, width, height, 1, apixel);
		}
	}
	return 0;
}

/* create 32bpp cursor */
int gdi_alpha_cursor_convert(uint8* alphaData, uint8* xorMask, uint8* andMask, int width, int height, int bpp, HPALETTE palette)
{
	int xpixel;
	int apixel;
	int i, j, jj;

	for (j = 0; j < height; j++)
	{
		jj = (bpp == 1) ? j : (height - 1) - j;
		for (i = 0; i < width; i++)
		{
			xpixel = gdi_get_pixel(xorMask, i, jj, width, height, bpp);
			xpixel = gdi_color(xpixel, bpp, 32, palette);
			apixel = gdi_get_pixel(andMask, i, jj, width, height, 1);

			if (apixel != 0)
			{
				if ((xpixel & 0xffffff) == 0xffffff)
				{
					/* use pattern (not solid black) for xor area */
					xpixel = (i & 1) == (j & 1);
					xpixel = xpixel ? 0xffffff : 0;
					xpixel |= 0xff000000;
				}
				else if (xpixel == 0xff000000)
				{
					xpixel = 0;
				}
			}

			gdi_set_pixel(alphaData, i, j, width, height, 32, xpixel);
		}
	}

	return 0;
}
