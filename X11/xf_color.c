/*
   FreeRDP: A Remote Desktop Protocol client.
   Channels

   Copyright (C) Jay Sorg 2009-2011

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

/*
  Valid color conversions
    8 -> 32   8 -> 24   8 -> 16   8 -> 15
    15 -> 32  15 -> 24  15 -> 16  15 -> 15
    16 -> 32  16 -> 24  16 -> 16
    24 -> 32  24 -> 24
    32 -> 32
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xf_types.h"

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
xf_color(xfInfo * xfi, int in_color, int in_bpp, int out_bpp)
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
			SPLIT32BGR(alpha, red, green, blue, in_color);
			break;
		case 24:
			SPLIT24BGR(red, green, blue, in_color);
			break;
		case 16:
			SPLIT16RGB(red, green, blue, in_color);
			break;
		case 15:
			SPLIT15RGB(red, green, blue, in_color);
			break;
		case 8:
			in_color &= 0xff;
			SPLIT24RGB(red, green, blue, xfi->colormap[in_color]);
			break;
		case 1:
			if (in_color != 0)
			{
				red = 0xff;
				green = 0xff;
				blue = 0xff;
			}
			break;
		default:
			printf("xf_color: bad in_bpp %d\n", in_bpp);
			break;
	}
	switch (out_bpp)
	{
		case 32:
			rv = MAKE32RGB(alpha, red, green, blue);
			break;
		case 24:
			rv = MAKE24RGB(red, green, blue);
			break;
		case 16:
			rv = MAKE16RGB(red, green, blue);
			break;
		case 15:
			rv = MAKE15RGB(red, green, blue);
			break;
		case 1:
			if ((red != 0) || (green != 0) || (blue != 0))
			{
				rv = 1;
			}
			break;
		default:
			printf("xf_color: bad out_bpp %d\n", out_bpp);
			break;
	}
	return rv;
}

int
xf_color_convert(xfInfo * xfi, rdpSet * settings, int color)
{
	return xf_color(xfi, color, settings->server_depth, xfi->bpp);
}

uint8 *
xf_image_convert(xfInfo * xfi, rdpSet * settings, int width, int height,
	uint8 * in_data)
{
	int red;
	int green;
	int blue;
	int index;
	int pixel;
	uint8 * out_data;
	uint8 * src8;
	uint8 * dst8;

	if ((settings->server_depth == 24) && (xfi->bpp == 32))
	{
		out_data = (uint8 *) malloc(width * height * 4);
		src8 = in_data;
		dst8 = out_data;
		for (index = width * height; index > 0; index--)
		{
			blue = *(src8++);
			green = *(src8++);
			red = *(src8++);
			pixel = MAKE24RGB(red, green, blue);
			*dst8++ = pixel & 0xff;
			*dst8++ = (pixel >> 8) & 0xff;
			*dst8++ = (pixel >> 16) & 0xff;
			*dst8++ = (pixel >> 24) & 0xff;
		}
		return out_data;
	}
	else if ((settings->server_depth == 16) && (xfi->bpp == 32))
	{
		out_data = (uint8 *) malloc(width * height * 4);
		src8 = in_data;
		dst8 = out_data;
		for (index = width * height; index > 0; index--)
		{
			pixel = *src8++;
			pixel |= (*src8++) << 8;
			SPLIT16RGB(red, green, blue, pixel);
			pixel = MAKE24RGB(red, green, blue);
			*dst8++ = pixel & 0xff;
			*dst8++ = (pixel >> 8) & 0xff;
			*dst8++ = (pixel >> 16) & 0xff;
			*dst8++ = (pixel >> 24) & 0xff;
		}
		return out_data;
	}
	else if ((settings->server_depth == 15) && (xfi->bpp == 32))
	{
		out_data = (uint8 *) malloc(width * height * 4);
		src8 = in_data;
		dst8 = out_data;
		for (index = width * height; index > 0; index--)
		{
			pixel = *src8++;
			pixel |= (*src8++) << 8;
			SPLIT15RGB(red, green, blue, pixel);
			pixel = MAKE24RGB(red, green, blue);
			*dst8++ = pixel & 0xff;
			*dst8++ = (pixel >> 8) & 0xff;
			*dst8++ = (pixel >> 16) & 0xff;
			*dst8++ = (pixel >> 24) & 0xff;
		}
		return out_data;
	}
	else if ((settings->server_depth == 8) && (xfi->bpp == 32))
	{
		out_data = (uint8 *) malloc(width * height * 4);
		src8 = in_data;
		dst8 = out_data;
		for (index = width * height; index > 0; index--)
		{
			pixel = *src8;
			src8++;
			pixel = xfi->colormap[pixel];
			*dst8++ = pixel & 0xff;
			*dst8++ = (pixel >> 8) & 0xff;
			*dst8++ = (pixel >> 16) & 0xff;
			*dst8++ = (pixel >> 24) & 0xff;
		}
		return out_data;
	}
	else if ((settings->server_depth == 15) && (xfi->bpp == 16))
	{
		out_data = (uint8 *) malloc(width * height * 2);
		src8 = in_data;
		dst8 = out_data;
		for (index = width * height; index > 0; index--)
		{
			pixel = *src8++;
			pixel |= (*src8++) << 8;
			SPLIT15RGB(red, green, blue, pixel);
			pixel = MAKE16RGB(red, green, blue);
			*dst8++ = pixel & 0xff;
			*dst8++ = (pixel >> 8) & 0xff;
		}
		return out_data;
	}
	else if ((settings->server_depth == 8) && (xfi->bpp == 16))
	{
		out_data = (uint8 *) malloc(width * height * 2);
		src8 = in_data;
		dst8 = out_data;
		for (index = width * height; index > 0; index--)
		{
			pixel = *src8;
			src8++;
			pixel = xfi->colormap[pixel];
			SPLIT24RGB(red, green, blue, pixel);
			pixel = MAKE16RGB(red, green, blue);
			*dst8++ = pixel & 0xff;
			*dst8++ = (pixel >> 8) & 0xff;
		}
		return out_data;
	}
	else if ((settings->server_depth == 8) && (xfi->bpp == 15))
	{
		out_data = (uint8 *) malloc(width * height * 2);
		src8 = in_data;
		dst8 = out_data;
		for (index = width * height; index > 0; index--)
		{
			pixel = *src8;
			src8++;
			pixel = xfi->colormap[pixel];
			SPLIT24RGB(red, green, blue, pixel);
			pixel = MAKE15RGB(red, green, blue);
			*dst8++ = pixel & 0xff;
			*dst8++ = (pixel >> 8) & 0xff;
		}
		return out_data;
	}
	return in_data;
}

RD_HPALETTE
xf_create_colormap(xfInfo * xfi, rdpSet * settings, RD_PALETTE * colors)
{
	int * colormap;
	int index;
	int red;
	int green;
	int blue;
	int count;

	colormap = (int *) malloc(sizeof(int) * 256);
	memset(colormap, 0, sizeof(int) * 256);
	count = colors->ncolors;
	if (count > 256)
	{
		count = 256;
	}
	for (index = count - 1; index >= 0; index--)
	{
		red = colors->colors[index].red;
		green = colors->colors[index].green;
		blue = colors->colors[index].blue;
		colormap[index] = MAKE24RGB(red, green, blue);
	}
	return (RD_HPALETTE) colormap;
}

int
xf_set_colormap(xfInfo * xfi, rdpSet * settings, RD_HPALETTE map)
{
	if (xfi->colormap != NULL)
	{
		free(xfi->colormap);
	}
	xfi->colormap = (int *) map;
	return 0;
}

/* create mono cursor */
int
xf_cursor_convert_mono(xfInfo * xfi, uint8 * src_data, uint8 * msk_data,
	uint8 * xormask, uint8 * andmask, int width, int height, int bpp)
{
	int i;
	int j;
	int jj;
	int xpixel;
	int apixel;

	for (j = 0; j < height; j++)
	{
		jj = (bpp == 1) ? j : (height - 1) - j;
		for (i = 0; i < width; i++)
		{
			xpixel = get_pixel(xormask, i, jj, width, height, bpp);
			xpixel = xf_color(xfi, xpixel, bpp, 24);
			apixel = get_pixel(andmask, i, jj, width, height, 1);
			if ((xpixel == 0xffffff) && (apixel != 0))
			{
				/* use pattern(not solid black) for xor area */
				xpixel = (i & 1) == (j & 1);
				apixel = 1;
			}
			else
			{
				xpixel = xpixel != 0;
				apixel = apixel == 0;
			}
			set_pixel(src_data, i, j, width, height, 1, xpixel);
			set_pixel(msk_data, i, j, width, height, 1, apixel);
		}
	}
	return 0;
}

/* create 32 bpp cursor */
int
xf_cursor_convert_alpha(xfInfo * xfi, uint8 * alpha_data,
	uint8 * xormask, uint8 * andmask, int width, int height, int bpp)
{
	int i;
	int j;
	int jj;
	int xpixel;
	int apixel;

	for (j = 0; j < height; j++)
	{
		jj = (bpp == 1) ? j : (height - 1) - j;
		for (i = 0; i < width; i++)
		{
			xpixel = get_pixel(xormask, i, jj, width, height, bpp);
			xpixel = xf_color(xfi, xpixel, bpp, 32);
			apixel = get_pixel(andmask, i, jj, width, height, 1);
			if (apixel != 0)
			{
				if ((xpixel & 0xffffff) == 0xffffff)
				{
					/* use pattern(not solid black) for xor area */
					xpixel = (i & 1) == (j & 1);
					xpixel = xpixel ? 0xffffff : 0;
					xpixel |= 0xff000000;
				}
				else if (xpixel == 0xff000000)
				{
					xpixel = 0;
				}
			}
			set_pixel(alpha_data, i, j, width, height, 32, xpixel);
		}
	}
	return 0;
}
