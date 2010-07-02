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

#include <stdio.h>
#include <stdlib.h>
#include "dfb_gdi.h"

HDC GetDC()
{
	HDC hDC = (HDC) malloc(sizeof(DC));
	return hDC;
}

HDC CreateCompatibleDC(HDC hdc)
{
	HDC hDC = (HDC) malloc(sizeof(DC));
	hDC->Bpp = hdc->Bpp;
	hDC->bpp = hdc->bpp;
	return hDC;
}

HBITMAP CreateBitmap(int nWidth, int nHeight, int cBitsPerPixel, char* data)
{
	HBITMAP hBitmap = (HBITMAP) malloc(sizeof(BITMAP));
	hBitmap->objectType = GDIOBJ_BITMAP;
	hBitmap->bpp = cBitsPerPixel;
	hBitmap->width = nWidth;
	hBitmap->height = nHeight;
	hBitmap->data = (char*) data;
	return hBitmap;
}

HBITMAP CreateCompatibleBitmap(HDC hdc, int nWidth, int nHeight)
{
	HBITMAP hBitmap = (HBITMAP) malloc(sizeof(BITMAP));
	hBitmap->objectType = GDIOBJ_BITMAP;
	hBitmap->width = nWidth;
	hBitmap->height = nHeight;
	return hBitmap;
}

HPEN CreatePen(int fnPenStyle, int nWidth, int crColor)
{
	HPEN hPen = (HPEN) malloc(sizeof(PEN));
	hPen->objectType = GDIOBJ_PEN;
	return hPen;
}

HBRUSH CreateSolidBrush(int crColor)
{
	HBRUSH hBrush = (HBRUSH) malloc(sizeof(BRUSH));
	hBrush->objectType = GDIOBJ_BRUSH;
	return hBrush;
}

HBRUSH CreatePatternBrush(HBITMAP hbmp)
{
	HBRUSH hBrush = (HBRUSH) malloc(sizeof(BRUSH));
	hBrush->objectType = GDIOBJ_BRUSH;
	return hBrush;
}

int SetRect(HRECT rc, int xLeft, int yTop, int xRight, int yBottom)
{
	rc->left = xLeft;
	rc->top = yTop;
	rc->right = xRight;
	rc->bottom = yBottom;
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
	return 1;
}

static void
gdi_copy_mem(char * d, char * s, int n)
{
	while (n & (~7))
	{
		*(d++) = *(s++);
		*(d++) = *(s++);
		*(d++) = *(s++);
		*(d++) = *(s++);
		*(d++) = *(s++);
		*(d++) = *(s++);
		*(d++) = *(s++);
		*(d++) = *(s++);
		n = n - 8;
	}
	while (n > 0)
	{
		*(d++) = *(s++);
		n--;
	}
}

static char *
gdi_get_bitmap_pointer(HDC hdcBmp, int x, int y)
{
	char * p;
	HBITMAP hBmp = (HBITMAP) hdcBmp->selectedObject;

	/*printf("x: %d y: %d width: %d height: %d bpp: %d\n",
	       x, y, hBmp->width, hBmp->height, hdcBmp->Bpp);*/
	
	if (x >= 0 && x < hBmp->width && y >= 0 && y < hBmp->height)
	{
		p = hBmp->data + (y * hBmp->width * hdcBmp->Bpp) + (x * hdcBmp->Bpp);
		return p;
	}
	else
	{
		return 0;
	}
}

int BitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, int rop)
{
	int i;
	char* srcp;
	char* dstp;

	/*printf("nXDest: %d nYDest: %d nWidth: %d nHeight: %d nXSrc: %d nYSrc: %d rop: 0x%X\n",
	       nXDest, nYDest, nWidth, nHeight, nXSrc, nYSrc, rop);*/

	if (rop == 0xC || rop == 0xCC || rop == 0x66 || rop == 0x88)
	{
		if (hdcDest->selectedObject->objectType == GDIOBJ_BITMAP && hdcSrc->selectedObject->objectType == GDIOBJ_BITMAP)
		{
			HBITMAP hSrcBmp = (HBITMAP) hdcSrc->selectedObject;

			srcp = (char *) (((unsigned int *) hSrcBmp->data) + nYSrc * hSrcBmp->width + nXSrc);
		
			for (i = 0; i < nHeight; i++)
			{
				dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + i);

				if (dstp != 0)
				{
					gdi_copy_mem(dstp, srcp, nWidth * hdcDest->Bpp);
					srcp += hSrcBmp->width * hdcDest->Bpp;
				}
			}
		}
	}
	
	return 1;
}

int SelectObject(HDC hdc, HGDIOBJ hgdiobj)
{
	if (hgdiobj->objectType == GDIOBJ_BITMAP)
	{
		//HBITMAP hBitmap = (HBITMAP) hgdiobj;
		hdc->selectedObject = hgdiobj;
	}
	else if (hgdiobj->objectType == GDIOBJ_PEN)
	{
		//HPEN hPen = (HPEN) hgdiobj;
		hdc->selectedObject = hgdiobj;
	}
	else if (hgdiobj->objectType == GDIOBJ_BRUSH)
	{
		//HBRUSH hBrush = (HBRUSH) hgdiobj;
		hdc->selectedObject = hgdiobj;
	}
	else if (hgdiobj->objectType == GDIOBJ_RECT)
	{
		//HRECT hRect = (HRECT) hgdiobj;
		hdc->selectedObject = hgdiobj;
	}
	else
	{
		/* Unknown GDI Object Type */
		return 0;
	}
	
	return 1;
}

int DeleteObject(HGDIOBJ hgdiobj)
{
	if (hgdiobj->objectType == GDIOBJ_BITMAP)
	{
		HBITMAP hBitmap = (HBITMAP) hgdiobj;
		free(hBitmap->data);
		free(hBitmap);
	}
	else if (hgdiobj->objectType == GDIOBJ_PEN)
	{
		HPEN hPen = (HPEN) hgdiobj;
		free(hPen);
	}
	else if (hgdiobj->objectType == GDIOBJ_BRUSH)
	{
		HBRUSH hBrush = (HBRUSH) hgdiobj;
		free(hBrush);
	}
	else if (hgdiobj->objectType == GDIOBJ_RECT)
	{
		HRECT hRect = (HRECT) hgdiobj;
		free(hRect);
	}
	else
	{
		/* Unknown GDI Object Type */
		free(hgdiobj);
		return 0;
	}
	
	return 1;
}

int DeleteDC(HDC hdc)
{
	free(hdc);
	return 1;
}
