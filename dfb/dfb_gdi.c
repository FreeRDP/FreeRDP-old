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
#include <string.h>
#include "dfb_gdi.h"

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
	
	if (x >= 0 && x < hBmp->width && y >= 0 && y < hBmp->height)
	{
		p = hBmp->data + (y * hBmp->width * hdcBmp->bytesPerPixel) + (x * hdcBmp->bytesPerPixel);
		return p;
	}
	else
	{
		return 0;
	}
}

HDC GetDC()
{
	HDC hDC = (HDC) malloc(sizeof(DC));
	return hDC;
}

HDC CreateCompatibleDC(HDC hdc)
{
	HDC hDC = (HDC) malloc(sizeof(DC));
	hDC->bytesPerPixel = hdc->bytesPerPixel;
	hDC->bitsPerPixel = hdc->bitsPerPixel;
	return hDC;
}

HBITMAP CreateBitmap(int nWidth, int nHeight, int cBitsPerPixel, char* data)
{
	HBITMAP hBitmap = (HBITMAP) malloc(sizeof(BITMAP));
	hBitmap->objectType = GDIOBJ_BITMAP;
	hBitmap->bitsPerPixel = cBitsPerPixel;
	hBitmap->width = nWidth;
	hBitmap->height = nHeight;
	hBitmap->data = (char*) data;
	return hBitmap;
}

HBITMAP CreateCompatibleBitmap(HDC hdc, int nWidth, int nHeight)
{
	HBITMAP hBitmap = (HBITMAP) malloc(sizeof(BITMAP));
	hBitmap->objectType = GDIOBJ_BITMAP;
	hBitmap->bytesPerPixel = hdc->bytesPerPixel;
	hBitmap->bitsPerPixel = hdc->bitsPerPixel;
	hBitmap->width = nWidth;
	hBitmap->height = nHeight;
	hBitmap->data = malloc(nWidth * nHeight * hBitmap->bytesPerPixel);
	return hBitmap;
}

HPEN CreatePen(int fnPenStyle, int nWidth, int crColor)
{
	HPEN hPen = (HPEN) malloc(sizeof(PEN));
	hPen->objectType = GDIOBJ_PEN;
	return hPen;
}

HBRUSH CreateSolidBrush(COLORREF crColor)
{
	HBRUSH hBrush = (HBRUSH) malloc(sizeof(BRUSH));
	hBrush->objectType = GDIOBJ_BRUSH;
	hBrush->style = BS_SOLID;
	hBrush->color = crColor;
	return hBrush;
}

HBRUSH CreatePatternBrush(HBITMAP hbmp)
{
	HBRUSH hBrush = (HBRUSH) malloc(sizeof(BRUSH));
	hBrush->objectType = GDIOBJ_BRUSH;
	hBrush->style = BS_PATTERN;
	hBrush->pattern = hbmp;
	return hBrush;
}

HRGN CreateRectRgn(int nLeftRect, int nTopRect, int nRightRect, int nBottomRect)
{
	HRGN hRgn = (HRGN) malloc(sizeof(RGN));
	hRgn->left = nLeftRect;
	hRgn->top = nTopRect;
	hRgn->right = nRightRect;
	hRgn->bottom = nBottomRect;
	return hRgn;
}

int SelectClipRgn(HDC hdc, HRGN hrgn)
{
	hdc->clippingRegion = hrgn;
	return 0;
}

COLORREF GetPixel(HDC hdc, int nXPos, int nYPos)
{
	return 0;
}

COLORREF SetPixel(HDC hdc, int X, int Y, COLORREF crColor)
{
	HBITMAP hBmp = (HBITMAP) hdc->selectedObject;
	*((COLORREF*)&(hBmp->data[X * hBmp->width * hdc->bytesPerPixel + Y * hdc->bytesPerPixel])) = crColor;
	return 0;
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
	int i;
	int j;
	HBITMAP hBmp = (HBITMAP) hdc->selectedObject;
	
	for (i = rect->top; i < rect->bottom; i++)
	{
		for (j = rect->left; j < rect->right; j++)
		{
			*((COLORREF*)&(hBmp->data[i * hBmp->width * hdc->bytesPerPixel + j * hdc->bytesPerPixel])) = hbr->color;
		}
	}
	
	return 1;
}

COLORREF GetBkColor(HDC hdc)
{
	return hdc->bkColor;
}

COLORREF SetBkColor(HDC hdc, COLORREF crColor)
{
	COLORREF previousBkColor = hdc->bkColor;
	hdc->bkColor = crColor;
	return previousBkColor;
}

COLORREF SetTextColor(HDC hdc, COLORREF crColor)
{
	COLORREF previousTextColor = hdc->textColor;
	hdc->textColor = crColor;
	return previousTextColor;
}

int SetBkMode(HDC hdc, int iBkMode)
{
	if (iBkMode == OPAQUE || iBkMode == TRANSPARENT)
		hdc->bkMode = iBkMode;
	else
		hdc->bkMode = OPAQUE; /* unknown background mode, default to sane value of OPAQUE */
	
	return 0;
}

int BitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, int rop)
{
	int i;
	char* srcp;
	char* dstp;

	printf("nXDest: %d nYDest: %d nWidth: %d nHeight: %d nXSrc: %d nYSrc: %d rop: 0x%X\n",
	       nXDest, nYDest, nWidth, nHeight, nXSrc, nYSrc, rop);

	/*
	 	0x00CC0020	SRCCOPY
		0x000C0324	SPna
		0x00000042	BLACKNESS
		0x00F00021	PATCOPY
	*/
	
	if (rop == SRCCOPY || rop == 0x000C0324)
	{
		HBITMAP hSrcBmp = (HBITMAP) hdcSrc->selectedObject;
		
		srcp = (char *) (((unsigned int *) hSrcBmp->data) + nYSrc * hSrcBmp->width + nXSrc);
		
		for (i = 0; i < nHeight; i++)
		{
			dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + i);

			if (dstp != 0)
			{
				gdi_copy_mem(dstp, srcp, nWidth * hdcDest->bytesPerPixel);
				srcp += hSrcBmp->width * hdcDest->bytesPerPixel;
			}
		}				
	}
	else if (rop == BLACKNESS)
	{
		for (i = 0; i < nHeight; i++)
		{
			dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + i);

			if (dstp != 0)
				memset(dstp, 0, nWidth * hdcDest->bytesPerPixel);
		}
	}
	else
	{
		printf("unknown rop: %X\n", rop);
	}
	
	return 1;
}

int PatBlt(HDC hdc, int nXLeft, int nYLeft, int nWidth, int nHeight, int rop)
{
	int i;
	char* srcp;
	char* dstp;

	/* PATCOPY, PATINVERT, DSTINVERT, BLACKNESS or WHITENESS */
	
	if (rop == PATCOPY)
	{
		HBRUSH hBrush;
		HBITMAP hPattern;
		
		hBrush = (HBRUSH) hdc->selectedObject;
		hPattern = hBrush->pattern;
		
		srcp = (char *) hPattern->data;
		
		for (i = 0; i < nHeight; i++)
		{
			dstp = gdi_get_bitmap_pointer(hdc, nXLeft, nYLeft + i);

			if (dstp != 0)
			{
				gdi_copy_mem(dstp, srcp, nWidth * hdc->bytesPerPixel);
				srcp += hPattern->width * hdc->bytesPerPixel;
			}
		}	
	}
	else
	{
		printf("PatBlt rop: 0x%X\n", rop);
	}
	
	return 1;
}

HGDIOBJ SelectObject(HDC hdc, HGDIOBJ hgdiobj)
{
	HGDIOBJ previousSelectedObject = hdc->selectedObject;

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
	
	return previousSelectedObject;
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
