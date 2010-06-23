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

#include "stdlib.h"
#include "dfb_gdi.h"

HDC GetDC()
{
	HDC hDC = (HDC) malloc(sizeof(DC));
	return HDC;
}

HDC CreateCompatibleDC(HDC hdc)
{
	HDC hDC = (HDC) malloc(sizeof(DC));
	return hDC;
}

HBITMAP CreateBitmap(int nWidth, int nHeight, int cBitsPerPixel, void* data)
{
	HBITMAP hBitmap = (HBITMAP) malloc(sizeof(BITMAP));
	return hPen;
}

HPEN CreatePen(int fnPenStyle, int nWidth, int crColor)
{
	HPEN hPen = (HPEN) malloc(sizeof(PEN));
	return hPen;
}

HBRUSH CreateSolidBrush(int crColor)
{
	HBRUSH hBrush = (HBRUSH) malloc(sizeof(BRUSH));
	return hBrush;
}

HBRUSH CreatePatternBrush(HBITMAP hbmp)
{
	HBRUSH hBrush = (HBRUSH) malloc(sizeof(BRUSH));
	return hBrush;
}

int SetRect(HRECT rc, int xLeft, int xTop, int xRight, int yBottom)
{
	rc->left = xLeft;
	rc->top = xTop;
	rc->right = xRight;
	rc->bottom = xBottom;
	return 1;
}

int CopyRect(HRECT dst, HRECT src)
{
	dst->left = src->left;
	dst->top = src->top;
	dst->right = src->right;
	dst->bottom = src->bottom;
	return 1;
}

int FillRect(HDC hdc, HRECT rect, HBRUSH hbr)
{
	return 1; /* 0 = failure */
}

int GetPixel(HDC hdc, int nXPos, int nYPos)
{
	return 0;
}

int SetPixel(HDC hdc, int X, int Y, int crColor)
{
	return 0;
}

int GetBkColor(HDC hdc)
{
	return 0;
}

int SetBkColor(HDC hdc, int crColor)
{
	return 0;
}

int PatBlt(HDC hdc, int nXLeft, int nXYLeft, int nWidth, int nHeight, int rop)
{
	/* Raster Operation is either PATCOPY, PATINVERT, DSTINVERT, BLACKNESS or WHITENESS */
}

int BitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, int hdcSrc, int nXSrc, int nYSrc, int rop)
{
	return 1; /* 0 = failure */
}

int DeleteDC(HDC hdc)
{
	free(hdc);
	return 1;
}
