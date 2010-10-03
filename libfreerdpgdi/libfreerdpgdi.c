/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   RDP GDI Adaption Layer

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
#include <string.h>
#include <stdlib.h>

#include <freerdp/freerdp.h>
#include "libfreerdpgdi.h"

#include "gdi_color.h"
#include "gdi_window.h"

HDC GetDC()
{
	HDC hDC = (HDC) malloc(sizeof(DC));
	hDC->bytesPerPixel = 4;
	hDC->bitsPerPixel = 32;
	hDC->drawMode = R2_COPYPEN;
	return hDC;
}

HDC CreateCompatibleDC(HDC hdc)
{
	HDC hDC = (HDC) malloc(sizeof(DC));
	hDC->bytesPerPixel = hdc->bytesPerPixel;
	hDC->bitsPerPixel = hdc->bitsPerPixel;
	hDC->drawMode = hdc->drawMode;
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

int CompareBitmaps(HBITMAP hBmp1, HBITMAP hBmp2)
{
	int x, y;
	char *p1, *p2;
	
	if (hBmp1->bitsPerPixel == hBmp2->bitsPerPixel)
	{
		if (hBmp1->width != hBmp2->width || hBmp1->height != hBmp2->height)
			return 0;

		p1 = hBmp1->data;
		p2 = hBmp2->data;
		
		for (y = 0; y < hBmp1->height; y++)
		{
			for (x = 0; x < hBmp1->width; x++)
			{
				if (*p1 != *p2)
					return 0;
				p1++;
				p2++;
				
				if (*p1 != *p2)
					return 0;
				p1++;
				p2++;

				if (*p1 != *p2)
					return 0;
				p1 += 2;
				p2 += 2;
			}
		}
	}
	else
	{
		return 0;
	}

	return 1;
}

HPEN CreatePen(int fnPenStyle, int nWidth, int crColor)
{
	HPEN hPen = (HPEN) malloc(sizeof(PEN));
	hPen->objectType = GDIOBJ_PEN;
	hPen->style = fnPenStyle;
	hPen->color = crColor;
	hPen->width = nWidth;
	return hPen;
}

HPALETTE CreatePalette(LOGPALETTE *lplgpl)
{
	HPALETTE hPalette = (HPALETTE) malloc(sizeof(PALETTE));
	hPalette->objectType = GDIOBJ_PALETTE;
	hPalette->logicalPalette = lplgpl;
	return hPalette;
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
	hRgn->objectType = GDIOBJ_REGION;
	hRgn->x = nLeftRect;
	hRgn->y = nTopRect;
	hRgn->w = nRightRect - nLeftRect;
	hRgn->h = nBottomRect - nTopRect;
	hRgn->null = 0;
	return hRgn;
}

HRECT CreateRect(int xLeft, int yTop, int xRight, int yBottom)
{
	HRECT hRect = (HRECT) malloc(sizeof(RECT));
	hRect->objectType = GDIOBJ_RECT;
	hRect->left = xLeft;
	hRect->top = yTop;
	hRect->right = xRight;
	hRect->bottom = yBottom;
	return hRect;
}

COLORREF GetPixel(HDC hdc, int nXPos, int nYPos)
{
	HBITMAP hBmp = (HBITMAP) hdc->selectedObject;
	COLORREF* colorp = (COLORREF*)&(hBmp->data[(nYPos * hBmp->width * hdc->bytesPerPixel) + nXPos * hdc->bytesPerPixel]);
	return (COLORREF) *colorp;
}

COLORREF SetPixel(HDC hdc, int X, int Y, COLORREF crColor)
{
	HBITMAP hBmp = (HBITMAP) hdc->selectedObject;
	*((COLORREF*)&(hBmp->data[(Y * hBmp->width * hdc->bytesPerPixel) + X * hdc->bytesPerPixel])) = crColor;
	return 0;
}

int SetROP2(HDC hdc, int fnDrawMode)
{
	int prevDrawMode = hdc->drawMode;
	hdc->drawMode = fnDrawMode;
	return prevDrawMode;
}

/* http://www.cs.toronto.edu/~smalik/418/tutorial2_bresenham.pdf */

#if 0
static void
bresenham(HDC hdc, int x1, int y1, int x2, int y2)
{
	int slope;
	int dx, dy;
	int d, x, y;
	int incE, incNE;

	/* reverse lines where x1 > x2 */
	if (x1 > x2)
	{
		bresenham(hdc, x2, y2, x1, y1);
		return;
	}

	dx = x2 - x1;
	dy = y2 - y1;

	/* adjust y-increment for negatively sloped lines */
	if (dy < 0)
	{
		slope = -1;
		dy = -dy;
	}
	else
	{
		slope = 1;
	}

	/* bresenham constants */
	incE = 2 * dy;
	incNE = 2 * dy - 2 * dx;
	d = 2 * dy - dx;
	y = y1;

	/* Blit */
	for (x = x1; x <= x2; x++)
	{
		/* TODO: apply correct binary raster operation */
		SetPixel(hdc, x, y, hdc->pen->color);
		
		if (d <= 0)
		{
			d += incE;
		}
		else
		{
			d += incNE;
			y += slope;
		}
	}
}
#endif

int LineTo(HDC hdc, int nXEnd, int nYEnd)
{	
	printf("LineTo: posX:%d posY:%d nXEnd:%d nYEnd:%d\n",
	       hdc->pen->posX, hdc->pen->posY, nXEnd, nYEnd);

	/*
	 * According to this MSDN article, LineTo uses a modified version of Bresenham:
	 * http://msdn.microsoft.com/en-us/library/dd145027(VS.85).aspx
	 *
	 * However, since I couldn't find the specifications of this modified algorithm,
	 * we're going to use the original Bresenham line drawing algorithm for now
	 */

	//bresenham(hdc, hdc->pen->posX, hdc->pen->posY, nXEnd, nYEnd);
	
	return 1;
}

int MoveTo(HDC hdc, int X, int Y)
{
	hdc->pen->posX = X;
	hdc->pen->posY = Y;
	return 1;
}

int SetRect(HRECT rc, int xLeft, int yTop, int xRight, int yBottom)
{
	rc->left = xLeft;
	rc->top = yTop;
	rc->right = xRight;
	rc->bottom = yBottom;
	return 1;
}

int SetRectRgn(HRGN hRgn, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect)
{
	hRgn->x = nLeftRect;
	hRgn->y = nTopRect;
	hRgn->w = nRightRect - nLeftRect;
	hRgn->h = nBottomRect - nTopRect;
	hRgn->null = 0;
	return 0;
}

int EqualRgn(HRGN hSrcRgn1, HRGN hSrcRgn2)
{
	if ((hSrcRgn1->x == hSrcRgn2->x) &&
	    (hSrcRgn1->y == hSrcRgn2->y) &&
	    (hSrcRgn1->w == hSrcRgn2->w) &&
	    (hSrcRgn1->h == hSrcRgn2->h))
	{
		return 1;
	}
	
	return 0;
}

int CopyRect(HRECT dst, HRECT src)
{
	dst->left = src->left;
	dst->top = src->top;
	dst->right = src->right;
	dst->bottom = src->bottom;
	return 1;
}

int PtInRect(HRECT rc, int x, int y)
{
	/* 
	 * points on the left and top sides are considered in,
	 * while points on the right and bottom sides are considered out
	 */
	
	if (x >= rc->left && x < rc->right)
	{
		if (y >= rc->top && y < rc->bottom)
		{
			return 1;
		}
	}
	
	return 0;
}

int FillRect(HDC hdc, HRECT rect, HBRUSH hbr)
{
	int x, y;
	HBITMAP hBmp;

	hBmp = (HBITMAP) hdc->selectedObject;

	for (y = rect->top; y < rect->bottom; y++)
	{
		for (x = rect->left; x < rect->right; x++)
		{
			*((COLORREF*)&(hBmp->data[y * hBmp->width * hdc->bytesPerPixel + x * hdc->bytesPerPixel])) = hbr->color;
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
		hdc->bkMode = OPAQUE;
	
	return 0;
}

static int BitBlt_BLACKNESS(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight)
{
	int y;
	char *dstp;
	
	for (y = 0; y < nHeight; y++)
	{
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
			memset(dstp, 0, nWidth * hdcDest->bytesPerPixel);
	}

	return 0;
}

static int BitBlt_WHITENESS(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight)
{
	int y;
	char *dstp;
	
	for (y = 0; y < nHeight; y++)
	{
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
			memset(dstp, 0xFF, nWidth * hdcDest->bytesPerPixel);
	}

	return 0;
}

static int BitBlt_SRCCOPY(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{
	int y;
	char *srcp;
	char *dstp;
	
	HBITMAP hSrcBmp = (HBITMAP) hdcSrc->selectedObject;
	srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc);
		
	for (y = 0; y < nHeight; y++)
	{
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			gdi_copy_mem(dstp, srcp, nWidth * hdcDest->bytesPerPixel);
			srcp += hSrcBmp->width * hdcDest->bytesPerPixel;
		}
	}

	return 0;
}

static int BitBlt_NOTSRCCOPY(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{
	int x, y;
	char *srcp;
	char *dstp;
	
	srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc);
		
	for (y = 0; y < nHeight; y++)
	{
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				*dstp = ~(*srcp);
				srcp++;
				dstp++;
					
				*dstp = ~(*srcp);
				srcp++;
				dstp++;

				*dstp = ~(*srcp);
				srcp += 2;
				dstp += 2;
			}
		}
	}

	return 0;
}

static int BitBlt_DSTINVERT(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight)
{
	int x, y;
	char *dstp;
		
	for (y = 0; y < nHeight; y++)
	{
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				*dstp = ~(*dstp);
				dstp++;
					
				*dstp = ~(*dstp);
				dstp++;

				*dstp = ~(*dstp);
				dstp += 2;
			}
		}
	}

	return 0;
}

static int BitBlt_SRCERASE(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{
	int x, y;
	char *srcp;
	char *dstp;
	
	srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc);
		
	for (y = 0; y < nHeight; y++)
	{
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				*dstp = *srcp & ~(*dstp);
				srcp++;
				dstp++;
					
				*dstp = *srcp & ~(*dstp);
				srcp++;
				dstp++;

				*dstp = *srcp & ~(*dstp);
				srcp += 2;
				dstp += 2;
			}
		}
	}

	return 0;
}

static int BitBlt_NOTSRCERASE(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{
	int x, y;
	char *srcp;
	char *dstp;
	
	srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc);
		
	for (y = 0; y < nHeight; y++)
	{
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				*dstp = ~(*srcp) & ~(*dstp);
				srcp++;
				dstp++;
					
				*dstp = ~(*srcp) & ~(*dstp);
				srcp++;
				dstp++;

				*dstp = ~(*srcp) & ~(*dstp);
				srcp += 2;
				dstp += 2;
			}
		}
	}

	return 0;
}

static int BitBlt_SRCINVERT(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{
	int x, y;
	char *srcp;
	char *dstp;
	
	srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc);
		
	for (y = 0; y < nHeight; y++)
	{
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				*dstp ^= *srcp;
				srcp++;
				dstp++;
					
				*dstp ^= *srcp;
				srcp++;
				dstp++;

				*dstp ^= *srcp;
				srcp += 2;
				dstp += 2;
			}
		}
	}

	return 0;
}

static int BitBlt_SRCAND(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{
	int x, y;
	char *srcp;
	char *dstp;
	
	srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc);
		
	for (y = 0; y < nHeight; y++)
	{
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				*dstp &= *srcp;
				srcp++;
				dstp++;
					
				*dstp &= *srcp;
				srcp++;
				dstp++;

				*dstp &= *srcp;
				srcp += 2;
				dstp += 2;
			}
		}
	}

	return 0;
}

static int BitBlt_SRCPAINT(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{
	int x, y;
	char *srcp;
	char *dstp;
	
	srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc);
		
	for (y = 0; y < nHeight; y++)
	{
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				*dstp |= *srcp;
				srcp++;
				dstp++;
					
				*dstp |= *srcp;
				srcp++;
				dstp++;

				*dstp |= *srcp;
				srcp += 2;
				dstp += 2;
			}
		}
	}

	return 0;
}

static int BitBlt_DSPDxax(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{
	int x, y;
	char *srcp;
	char *dstp;
	
	/* DSPDxax, used to draw glyphs */
	HBITMAP hSrcBmp = (HBITMAP) hdcSrc->selectedObject;
	srcp = (char *) (((unsigned int *) hSrcBmp->data) + nYSrc * hSrcBmp->width + nXSrc);

	for (y = 0; y < nHeight; y++)
	{
		for (x = 0; x < nWidth; x++)
		{
			if (gdi_is_mono_pixel_set(srcp, x, y, hSrcBmp->width))
			{
				dstp = gdi_get_bitmap_pointer(hdcDest, nXDest + x, nYDest + y);

				if (dstp != 0)
					memset(dstp, hdcDest->textColor, hdcDest->bytesPerPixel);
			}
		}
	}

	return 0;
}

static int BitBlt_PATCOPY(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight)
{
#if 0
	int y;
	char *srcp;
	char *dstp;
	
	HBITMAP hPattern = hdcDest->brush->pattern;
	srcp = (char*) hPattern->data;
		
	for (y = 0; y < nHeight; y++)
	{
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			gdi_copy_mem(dstp, srcp, nWidth * hdcDest->bytesPerPixel);
			srcp += hPattern->width * hdcDest->bytesPerPixel;
		}
	}
#endif
	return 0;
}

static int BitBlt_PATINVERT(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight)
{
#if 0
	int x, y;
	char *srcp;
	char *dstp;

	HBITMAP hPattern = hdcDest->brush->pattern;
	srcp = (char*) hPattern->data;
	
	for (y = 0; y < nHeight; y++)
	{
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				*dstp ^= *srcp;
				srcp++;
				dstp++;
					
				*dstp ^= *srcp;
				srcp++;
				dstp++;

				*dstp ^= *srcp;
				srcp += 2;
				dstp += 2;
			}
		}
	}
#endif
	return 0;
}

int BitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, int rop)
{
	switch (rop)
	{
		case BLACKNESS:
			return BitBlt_BLACKNESS(hdcDest, nXDest, nYDest, nWidth, nHeight);
			break;

		case WHITENESS:
			return BitBlt_WHITENESS(hdcDest, nXDest, nYDest, nWidth, nHeight);
			break;
			
		case SPna:
		case SRCCOPY:
			return BitBlt_SRCCOPY(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case DSPDxax:
			return BitBlt_DSPDxax(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;
			
		case NOTSRCCOPY:
			return BitBlt_NOTSRCCOPY(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case DSTINVERT:
			return BitBlt_DSTINVERT(hdcDest, nXDest, nYDest, nWidth, nHeight);
			break;

		case SRCERASE:
			return BitBlt_SRCERASE(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case NOTSRCERASE:
			return BitBlt_NOTSRCERASE(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case SRCINVERT:
			return BitBlt_SRCINVERT(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case SRCAND:
			return BitBlt_SRCAND(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case SRCPAINT:
			return BitBlt_SRCPAINT(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		/*case MERGEPAINT:
			break;*/

		/*case MERGECOPY:
			break;*/

		case PATCOPY:
			return BitBlt_PATCOPY(hdcDest, nXDest, nYDest, nWidth, nHeight);
			break;

		case PATINVERT:
			return BitBlt_PATCOPY(hdcDest, nXDest, nYDest, nWidth, nHeight);
			break;

		/*case PATPAINT:
			break;*/
	}
	
	printf("BitBlt: unknown rop: 0x%08X\n", rop);
	return 1;
}

int PatBlt(HDC hdc, int nXLeft, int nYLeft, int nWidth, int nHeight, int rop)
{
	switch (rop)
	{
		case PATCOPY:
			return BitBlt_PATCOPY(hdc, nXLeft, nYLeft, nWidth, nHeight);
			break;

		case PATINVERT:
			return BitBlt_PATINVERT(hdc, nXLeft, nYLeft, nWidth, nHeight);
			break;
			
		case DSTINVERT:
			return BitBlt_DSTINVERT(hdc, nXLeft, nYLeft, nWidth, nHeight);
			break;

		case BLACKNESS:
			return BitBlt_BLACKNESS(hdc, nXLeft, nYLeft, nWidth, nHeight);
			break;

		case WHITENESS:
			return BitBlt_WHITENESS(hdc, nXLeft, nYLeft, nWidth, nHeight);
			break;
	}
	
	printf("PatBlt: unknown rop: 0x%08X", rop);
	return 1;
}

HGDIOBJ SelectObject(HDC hdc, HGDIOBJ hgdiobj)
{
	HGDIOBJ previousSelectedObject = hdc->selectedObject;

	if (hgdiobj->objectType == GDIOBJ_BITMAP)
	{
		hdc->selectedObject = hgdiobj;
	}
	else if (hgdiobj->objectType == GDIOBJ_PEN)
	{
		previousSelectedObject = (HGDIOBJ) hdc->pen;
		hdc->pen = (HPEN) hgdiobj;
	}
	else if (hgdiobj->objectType == GDIOBJ_BRUSH)
	{
		previousSelectedObject = (HGDIOBJ) hdc->brush;
		hdc->brush = (HBRUSH) hgdiobj;
	}
	else if (hgdiobj->objectType == GDIOBJ_REGION)
	{
		hdc->selectedObject = hgdiobj;
	}
	else if (hgdiobj->objectType == GDIOBJ_RECT)
	{
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
	if (hgdiobj == NULL)
		return 0;
	
	if (hgdiobj->objectType == GDIOBJ_BITMAP)
	{
		HBITMAP hBitmap = (HBITMAP) hgdiobj;

		if (hBitmap->data != NULL)
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
	else if (hgdiobj->objectType == GDIOBJ_PALETTE)
	{
		HPALETTE hPalette = (HPALETTE) hgdiobj;

		if (hPalette->logicalPalette != NULL)
		{
			if (hPalette->logicalPalette->entries != NULL)
				free(hPalette->logicalPalette->entries);

			free(hPalette->logicalPalette);
		}
		
		free(hPalette);
	}
	else if (hgdiobj->objectType == GDIOBJ_REGION)
	{
		free(hgdiobj);
	}
	else if (hgdiobj->objectType == GDIOBJ_RECT)
	{
		free(hgdiobj);
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
