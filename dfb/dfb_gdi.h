/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   DirectFB GDI Adapation Layer

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

#ifndef __DFB_GDI_H
#define __DFB_GDI_H

/* For more information, see [MS-RDPEGDI].pdf */

/* Binary Raster Operations (ROP2) */
#define R2_BLACK		0x01  /* 0    */
#define R2_NOTMERGEPEN		0x02  /* DPon */
#define R2_MASKNOTPEN		0x03  /* DPna */
#define R2_NOTCOPYPEN		0x04  /* PN   */
#define R2_MASKPENNOT		0x05  /* PDna */
#define R2_NOT			0x06  /* Dn   */
#define R2_XORPEN		0x07  /* DPx  */
#define R2_NOTMASKPEN		0x08  /* DPan */
#define R2_MASKPEN		0x09  /* DPa  */
#define R2_NOTXORPEN		0x0A  /* DPxn */
#define R2_NOP			0x0B  /* D    */
#define R2_MERGENOTPEN		0x0C  /* DPno */
#define R2_COPYPEN		0x0D  /* P    */
#define R2_MERGEPENNOT		0x0E  /* PDno */
#define R2_MERGEPEN		0x0F  /* DPo  */
#define R2_WHITE		0x10  /* 1    */

/* Ternary Raster Operations (ROP3) */
#define SRCCOPY			0x00CC0020 /* D = S       */
#define SRCPAINT		0x00EE0086 /* D = S | D   */
#define SRCAND			0x008800C6 /* D = S & D   */
#define SRCINVERT		0x00660046 /* D = S ^ D   */
#define SRCERASE		0x00440328 /* D = S & !D  */
#define NOTSRCCOPY		0x00330008 /* D = !S      */
#define NOTSRCERASE		0x001100A6 /* D = !S & !D */
#define MERGECOPY		0x00C000CA /* D = S & P   */
#define MERGEPAINT		0x00BB0226 /* D = !S | D  */
#define PATCOPY			0x00F00021 /* D = P       */
#define PATPAINT		0x00FB0A09 /* D = DPSnoo  */
#define PATINVERT		0x005A0049 /* D = P ^ D   */
#define DSTINVERT		0x00550009 /* D = !D      */
#define BLACKNESS		0x00000042 /* D = BLACK   */
#define WHITENESS		0x00FF0062 /* D = WHITE   */

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

struct _DC
{
	unsigned int bkColor;
};
typedef struct _DC DC;
typedef DC* HDC;

struct _RECT
{
	unsigned int left;
	unsigned int top;
	unsigned int right;
	unsigned int bottom;
};
typedef struct _RECT RECT;
typedef RECT* HRECT;

struct _PEN
{
	unsigned int style;
	unsigned int width;
};
typedef struct _PEN PEN;
typedef PEN* HPEN;

struct _BRUSH
{
	unsigned int style;
};
typedef struct _BRUSH BRUSH;
typedef BRUSH* HBRUSH;

struct _BITMAP
{
	unsigned int width;
	unsigned int height;
	unsigned int bitsPerPixel;
};
typedef struct _BITMAP BITMAP;
typedef BITMAP* HBITMAP;

HDC GetDC();
HDC CreateCompatibleDC(HDC hdc);
HBITMAP CreateBitmap(int nWidth, int nHeight, int cBitsPerPixel, void* data);
HPEN CreatePen(int fnPenStyle, int nWidth, int crColor);
HBRUSH CreateSolidBrush(int crColor);
HBRUSH CreatePatternBrush(HBITMAP hbmp);
int SetRect(HRECT rc, int xLeft, int xTop, int xRight, int yBottom);
int CopyRect(HRECT dst, HRECT src);
int FillRect(HDC hdc, HRECT rect, HBRUSH hbr);
int GetPixel(HDC hdc, int nXPos, int nYPos);
int SetPixel(HDC hdc, int X, int Y, int crColor);
int GetBkColor(HDC hdc);
int SetBkColor(HDC hdc, int crColor);
int PatBlt(HDC hdc, int nXLeft, int nXYLeft, int nWidth, int nHeight, int rop);
int BitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, int hdcSrc, int nXSrc, int nYSrc, int rop);
int DeleteDC(HDC hdc);

#endif /* __DFB_GDI_H */
