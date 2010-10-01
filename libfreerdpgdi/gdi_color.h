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

#ifndef __GDI_COLOR_H
#define __GDI_COLOR_H

#include "libfreerdpgdi.h"

/* COLORREF (RGB 24) */

#define RGB(_r, _g, _b)  \
	(_r << 16) | (_g << 8) | _b;

#define GetRGB(_r, _g, _b, _p) \
	_r = (_p & 0xFF0000) >> 16; \
	_g = (_p & 0xFF00) >> 8; \
	_b = (_p & 0xFF); \

/* RGB 15 (RGB_555) */

#define RGB15(_r, _g, _b)  \
	(_r << 10) | (_g << 5) | _b;

#define GetRGB15(_r, _g, _b, _p) \
	_r = (_p & 0x7C00) >> 10; \
	_g = (_p & 0x3E0) >> 5; \
	_b = (_p & 0x1F);

/* BGR 15 (BGR_555) */

#define BGR15(_r, _g, _b)  \
	(_b << 10) | (_g << 5) | _r;

#define GetBGR15(_r, _g, _b, _p) \
	_b = ((_p << 3) & 0xF8) | ((_p >> 2) & 0x7); \
	_g = ((_p >> 2) & 0xF8) | ((_p >> 8) & 0x7); \
	_r = ((_p >> 7) & 0xF8) | ((_p >> 12) & 0x7);

/* RGB 16 (RGB_565) */

#define RGB16(_r, _g, _b)  \
	(_r << 11) | (_g << 5) | _b;

#define GetRGB16(_r, _g, _b, _p) \
	_r = (_p & 0xF800) >> 11; \
	_g = (_p & 0x7E0) >> 5; \
	_b = (_p & 0x1F);

/* BGR 16 (BGR_565) */

#define BGR16(_r, _g, _b)  \
	(_b << 11) | (_g << 5) | _r;

#define GetBGR16(_r, _g, _b, _p) \
	_b = ((_p << 3) & 0xF8) | ((_p >> 2) & 0x7); \
	_g = ((_p >> 3) & 0xFC) | ((_p >> 9) & 0x3); \
	_r = ((_p >> 8) & 0xF8) | ((_p >> 13) & 0x7);

/* RGB 24 (RGB_888) */

#define RGB24(_r, _g, _b)  \
	(_r << 16) | (_g << 8) | _b;

#define GetRGB24(_r, _g, _b, _p) \
	_r = (_p & 0xFF0000) >> 16; \
	_g = (_p & 0xFF00) >> 8; \
	_b = (_p & 0xFF);

/* BGR 24 (BGR_888) */

#define BGR24(_r, _g, _b)  \
	(_b << 16) | (_g << 8) | _r;

#define GetBGR24(_r, _g, _b, _p) \
	_b = (_p & 0xFF0000) >> 16; \
	_g = (_p & 0xFF00) >> 8; \
	_r = (_p & 0xFF);

/* RGB 32 (ARGB_8888), alpha ignored */

#define RGB32(_r, _g, _b)  \
	(_r << 16) | (_g << 8) | _b;

#define GetRGB32(_r, _g, _b, _p) \
	_r = (_p & 0xFF0000) >> 16; \
	_g = (_p & 0xFF00) >> 8; \
	_b = (_p & 0xFF);

/* ARGB 32 (ARGB_8888) */

#define ARGB32(_a,_r, _g, _b)  \
	(_a << 24) | (_r << 16) | (_g << 8) | _b;

#define GetARGB32(_a, _r, _g, _b, _p) \
	_a = (_p & 0xFF000000) >> 24; \
	_r = (_p & 0xFF0000) >> 16; \
	_g = (_p & 0xFF00) >> 8; \
	_b = (_p & 0xFF);

/* BGR 32 (ABGR_8888), alpha ignored */

#define BGR32(_r, _g, _b)  \
	(_b << 16) | (_g << 8) | _r;

#define GetBGR32(_r, _g, _b, _p) \
	_b = (_p & 0xFF0000) >> 16; \
	_g = (_p & 0xFF00) >> 8; \
	_r = (_p & 0xFF);

/* BGR 32 (ABGR_8888) */

#define ABGR32(_a, _r, _g, _b)  \
	(_a << 24) | (_b << 16) | (_g << 8) | _r;

#define GetABGR32(_a, _r, _g, _b, _p) \
	_a = (_p & 0xFF000000) >> 24; \
	_b = (_p & 0xFF0000) >> 16; \
	_g = (_p & 0xFF00) >> 8; \
	_r = (_p & 0xFF);

unsigned int gdi_make_colorref(PIXEL *pixel);
void gdi_split_colorref(unsigned int colorref, PIXEL *pixel);
void gdi_color_convert(PIXEL *pixel, int color, int bpp, HPALETTE palette);
char* gdi_image_convert(char* srcData, int width, int height, int srcBpp, int dstBpp, HPALETTE palette);

#endif /* __GDI_COLOR_H */
