/*
   FreeRDP: A Remote Desktop Protocol client.
   GDI Library

   Copyright 2010-2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef __GDI_H
#define __GDI_H

#include "color.h"
#include <freerdp/freerdp.h>
#include <freerdp/utils/debug.h>

/* For more information, see [MS-RDPEGDI] */

/* Binary Raster Operations (ROP2) */
#define R2_BLACK		0x01  /* D = 0 */
#define R2_NOTMERGEPEN		0x02  /* D = ~(D | P) */
#define R2_MASKNOTPEN		0x03  /* D = D & ~P */
#define R2_NOTCOPYPEN		0x04  /* D = ~P */
#define R2_MASKPENNOT		0x05  /* D = P & ~D */
#define R2_NOT			0x06  /* D = ~D */
#define R2_XORPEN		0x07  /* D = D ^ P */
#define R2_NOTMASKPEN		0x08  /* D = ~(D & P) */
#define R2_MASKPEN		0x09  /* D = D & P */
#define R2_NOTXORPEN		0x0A  /* D = ~(D ^ P) */
#define R2_NOP			0x0B  /* D = D */
#define R2_MERGENOTPEN		0x0C  /* D = D | ~P */
#define R2_COPYPEN		0x0D  /* D = P */
#define R2_MERGEPENNOT		0x0E  /* D = P | ~D */
#define R2_MERGEPEN		0x0F  /* D = P | D */
#define R2_WHITE		0x10  /* D = 1 */

/* Ternary Raster Operations (ROP3) */
#define SRCCOPY			0x00CC0020 /* D = S */
#define SRCPAINT		0x00EE0086 /* D = S | D	*/
#define SRCAND			0x008800C6 /* D = S & D	*/
#define SRCINVERT		0x00660046 /* D = S ^ D	*/
#define SRCERASE		0x00440328 /* D = S & ~D */
#define NOTSRCCOPY		0x00330008 /* D = ~S */
#define NOTSRCERASE		0x001100A6 /* D = ~S & ~D */
#define MERGECOPY		0x00C000CA /* D = S & P	*/
#define MERGEPAINT		0x00BB0226 /* D = ~S | D */
#define PATCOPY			0x00F00021 /* D = P */
#define PATPAINT		0x00FB0A09 /* D = D | (P | ~S) */
#define PATINVERT		0x005A0049 /* D = P ^ D	*/
#define DSTINVERT		0x00550009 /* D = ~D */
#define BLACKNESS		0x00000042 /* D = 0 */
#define WHITENESS		0x00FF0062 /* D = 1 */
#define DSPDxax			0x00E20746 /* D = (S & P) | (~S & D) */
#define SPna			0x000C0324 /* D = S & ~P */
#define DSna			0x00220326 /* D = D & ~S */

/* Brush Styles */
#define BS_SOLID		0x00
#define BS_NULL			0x01
#define BS_HATCHED		0x02
#define BS_PATTERN		0x03

/* Hatch Patterns */
#define HS_HORIZONTAL		0x00
#define HS_VERTICAL		0x01
#define HS_FDIAGONAL		0x02
#define HS_BDIAGONAL		0x03
#define HS_CROSS		0x04
#define HS_DIAGCROSS		0x05

/* Pen Styles */
#define PS_SOLID		0x00
#define PS_DASH			0x01
#define PS_NULL			0x05

/* GDI Object Types */
#define GDIOBJ_BITMAP		0x00
#define GDIOBJ_PEN		0x01
#define GDIOBJ_PALETTE		0x02
#define GDIOBJ_BRUSH		0x03
#define GDIOBJ_RECT		0x04
#define GDIOBJ_REGION		0x04

/* Background Modes */
#define OPAQUE			0x00000001
#define TRANSPARENT		0x00000002

struct _GDIOBJ
{
	unsigned char objectType;
};
typedef struct _GDIOBJ GDIOBJ;
typedef GDIOBJ* HGDIOBJ;

/* RGB encoded as 0x00BBGGRR */
typedef unsigned int COLORREF;
typedef COLORREF* LPCOLORREF;

struct _RECT
{
	unsigned char objectType;
	unsigned int left;
	unsigned int top;
	unsigned int right;
	unsigned int bottom;
};
typedef struct _RECT RECT;
typedef RECT* HRECT;

struct _RGN
{
	unsigned char objectType;
	int x; /* left */
	int y; /* top */
	int w; /* width */
	int h; /* height */
	int null; /* null region */
};
typedef struct _RGN RGN;
typedef RGN* HRGN;

struct _BITMAP
{
	unsigned char objectType;
	unsigned int bytesPerPixel;
	unsigned int bitsPerPixel;
	unsigned int width;
	unsigned int height;
	unsigned int scanline;
	uint8* data;
};
typedef struct _BITMAP BITMAP;
typedef BITMAP* HBITMAP;

struct _PEN
{
	unsigned char objectType;
	unsigned int style;
	unsigned int width;
	unsigned int posX;
	unsigned int posY;
	COLORREF color;
};
typedef struct _PEN PEN;
typedef PEN* HPEN;

struct _PALETTEENTRY
{
	uint8 red;
	uint8 green;
	uint8 blue;
};
typedef struct _PALETTEENTRY PALETTEENTRY;

struct _PALETTE
{
	uint16 count;
	PALETTEENTRY *entries;
};
typedef struct _PALETTE PALETTE;
typedef PALETTE* HPALETTE;

struct _POINT
{
	unsigned int x;
	unsigned int y;
};
typedef struct _POINT POINT;
typedef POINT* HPOINT;

struct _BRUSH
{
	unsigned char objectType;
	unsigned int style;
	HBITMAP pattern;
	COLORREF color;
};
typedef struct _BRUSH BRUSH;
typedef BRUSH* HBRUSH;

struct _WND
{
	HRGN invalid;
};
typedef struct _WND WND;
typedef WND* HWND;

struct _DC
{
	HGDIOBJ selectedObject;
	unsigned int bytesPerPixel;
	unsigned int bitsPerPixel;
	COLORREF bkColor;
	COLORREF textColor;
	HBRUSH brush;
	HRGN clip;
	HPEN pen;
	HWND hwnd;
	int drawMode;
	int bkMode;
	int alpha;
	int invert;
	int rgb555;
};
typedef struct _DC DC;
typedef DC* HDC;

struct _gdi_bitmap
{
	HDC hdc;
	HBITMAP bitmap;
	HBITMAP org_bitmap;
};
typedef struct _gdi_bitmap gdi_bitmap;

struct _GDI
{
	int width;
	int height;
	int dstBpp;
	int srcBpp;
	int cursor_x;
	int cursor_y;
	int bytesPerPixel;

	HDC hdc;
	HCLRCONV clrconv;
	gdi_bitmap *primary;
	gdi_bitmap *drawing;
	uint8* primary_buffer;
	COLORREF textColor;
	void * rfx_context;
	gdi_bitmap *tile;
};
typedef struct _GDI GDI;

unsigned int gdi_rop3_code(unsigned char code);
void gdi_copy_mem(uint8 *d, uint8 *s, int n);
void gdi_copy_memb(uint8 *d, uint8 *s, int n);
uint8* gdi_get_bitmap_pointer(HDC hdcBmp, int x, int y);
uint8* gdi_get_brush_pointer(HDC hdcBrush, int x, int y);
int gdi_is_mono_pixel_set(uint8* data, int x, int y, int width);
int gdi_init(rdpInst * inst, uint32 flags);
gdi_bitmap* gdi_bitmap_new(GDI *gdi, int width, int height, int bpp, uint8* data);
void gdi_bitmap_free(gdi_bitmap *gdi_bmp);

#define SET_GDI(_inst, _gdi) (_inst)->param2 = _gdi
#define GET_GDI(_inst) ((GDI*) ((_inst)->param2))

#ifdef WITH_DEBUG_GDI
#define DEBUG_GDI(fmt, ...) DEBUG_CLASS(GDI, fmt, ...)
#else
#define DEBUG_GDI(fmt, ...) DEBUG_NULL(fmt, ...)
#endif

#endif /* __GDI_H */
