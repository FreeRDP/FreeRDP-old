
/*
  Valid colour conversions
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
#include "xf_win.h"
#include "xf_event.h"

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

#define MAKE32RGB(_red, _green, _blue) (_red << 16) | (_green << 8) | _blue;

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
			return MAKE32RGB(red, green, blue);
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
xf_colour(xfInfo * xfi, int in_colour, int in_bpp, int out_bpp)
{
	int red;
	int green;
	int blue;
	int rv;

	red = 0;
	green = 0;
	blue = 0;
	rv = 0;
	switch (in_bpp)
	{
		case 32:
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
			SPLIT24RGB(red, green, blue, xfi->colourmap[in_colour]);
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
			printf("xf_colour: bad in_bpp %d\n", in_bpp);
			break;
	}
	switch (out_bpp)
	{
		case 32:
		case 24:
			rv = MAKE32RGB(red, green, blue);
			break;
		case 1:
			if ((red != 0) || (green != 0) || (blue != 0))
			{
				rv = 1;
			}
			break;
		default:
			printf("xf_colour: bad out_bpp %d\n", out_bpp);
			break;
	}
	return rv;
}

int
xf_colour_convert(xfInfo * xfi, rdpSet * settings, int colour)
{
	return xf_colour(xfi, colour, settings->server_depth, xfi->bpp);
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
	uint16 * src16;
	uint32 * dst32;

	if ((settings->server_depth == 24) && (xfi->bpp == 32))
	{
		out_data = (uint8 *) malloc(width * height * 4);
		src8 = in_data;
		dst32 = (uint32 *) out_data;
		for (index = width * height; index > 0; index--)
		{
			blue = *(src8++);
			green = *(src8++);
			red = *(src8++);
			pixel = MAKE32RGB(red, green, blue);
			*dst32 = pixel;
			dst32++;
		}
		return out_data;
	}
	else if ((settings->server_depth == 16) && (xfi->bpp == 32))
	{
		out_data = (uint8 *) malloc(width * height * 4);
		src16 = (uint16 *) in_data;
		dst32 = (uint32 *) out_data;
		for (index = width * height; index > 0; index--)
		{
			pixel = *src16;
			src16++;
			SPLIT16RGB(red, green, blue, pixel);
			pixel = MAKE32RGB(red, green, blue);
			*dst32 = pixel;
			dst32++;
		}
		return out_data;
	}
	else if ((settings->server_depth == 15) && (xfi->bpp == 32))
	{
		out_data = (uint8 *) malloc(width * height * 4);
		src16 = (uint16 *) in_data;
		dst32 = (uint32 *) out_data;
		for (index = width * height; index > 0; index--)
		{
			pixel = *src16;
			src16++;
			SPLIT15RGB(red, green, blue, pixel);
			pixel = MAKE32RGB(red, green, blue);
			*dst32 = pixel;
			dst32++;
		}
		return out_data;
	}
	else if ((settings->server_depth == 8) && (xfi->bpp == 32))
	{
		out_data = (uint8 *) malloc(width * height * 4);
		src8 = in_data;
		dst32 = (uint32 *) out_data;
		for (index = width * height; index > 0; index--)
		{
			pixel = *src8;
			src8++;
			pixel = xfi->colourmap[pixel];
			*dst32 = pixel;
			dst32++;
		}
		return out_data;
	}
	return in_data;
}

RD_HCOLOURMAP
xf_create_colourmap(xfInfo * xfi, rdpSet * settings, RD_COLOURMAP * colours)
{
	int * colourmap;
	int index;
	int red;
	int green;
	int blue;
	int count;

	colourmap = (int *) malloc(sizeof(int) * 256);
	memset(colourmap, 0, sizeof(int) * 256);
	count = colours->ncolours;
	if (count > 256)
	{
		count = 256;
	}
	if (xfi->bpp == 32)
	{
		for (index = count - 1; index >= 0; index--)
		{
			red = colours->colours[index].red;
			green = colours->colours[index].green;
			blue = colours->colours[index].blue;
			colourmap[index] = MAKE32RGB(red, green, blue);
		}
	}
	return (RD_HCOLOURMAP) colourmap;
}

int
xf_set_colourmap(xfInfo * xfi, rdpSet * settings, RD_HCOLOURMAP map)
{
	if (xfi->colourmap != NULL)
	{
		free(xfi->colourmap);
	}
	xfi->colourmap = (int *) map;
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
			xpixel = xf_colour(xfi, xpixel, bpp, 1);
			apixel = get_pixel(andmask, i, jj, width, height, 1);
			if ((xpixel != 0) && (apixel != 0))
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
	uint8 * xormask, uint8 * andmask, int width, int height,
        int bpp, int server_depth)
{
	int i;
	int j;
	int jj;
	int xpixel;
	int apixel;
	int lbpp;

	lbpp = bpp == 1 ? bpp : server_depth;
	for (j = 0; j < height; j++)
	{
		jj = (bpp == 1) ? j : (height - 1) - j;
		for (i = 0; i < width; i++)
		{
			xpixel = get_pixel(xormask, i, jj, width, height, bpp);
			xpixel = xf_colour(xfi, xpixel, lbpp, 32);
			apixel = get_pixel(andmask, i, jj, width, height, 1);
			if ((xpixel != 0) && (apixel != 0))
			{
				/* use pattern(not solid black) for xor area */
				xpixel = (i & 1) == (j & 1);
				xpixel = xpixel ? 0xffffff : 0;
				xpixel |= 0xff000000;
			}
			else
			{
				xpixel |= apixel ? 0 : 0xff000000;
			}
			set_pixel(alpha_data, i, j, width, height, 32, xpixel);
		}
	}
	return 0;
}
