/*
   FreeRDP: A Remote Desktop Protocol client.
   RDP GDI Adaption Layer

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
#include "libfreerdpgdi.h"

#include "gdi_color.h"
#include "gdi_window.h"
#include "gdi_32bpp.h"
#include "gdi_16bpp.h"

/**
 * Get the current device context (a new one is created each time).
 * http://msdn.microsoft.com/en-us/library/dd144871/
 * @return current device context
 */

HDC GetDC()
{
	HDC hDC = (HDC) malloc(sizeof(DC));
	hDC->bytesPerPixel = 4;
	hDC->bitsPerPixel = 32;
	hDC->drawMode = R2_COPYPEN;
	hDC->clip = CreateRectRgn(0, 0, 0, 0);
	hDC->clip->null = 1;
	hDC->hwnd = NULL;
	return hDC;
}

/**
 * Create a new device context compatible with the given device context.
 * http://msdn.microsoft.com/en-us/library/dd183489/
 * @param hdc device context
 * @return new compatible device context
 */

HDC CreateCompatibleDC(HDC hdc)
{
	HDC hDC = (HDC) malloc(sizeof(DC));
	hDC->bytesPerPixel = hdc->bytesPerPixel;
	hDC->bitsPerPixel = hdc->bitsPerPixel;
	hDC->drawMode = hdc->drawMode;
	hDC->clip = CreateRectRgn(0, 0, 0, 0);
	hDC->clip->null = 1;
	hDC->hwnd = NULL;
	return hDC;
}

/**
 * Create a new bitmap with the given width, height, color format and pixel buffer.
 * http://msdn.microsoft.com/en-us/library/dd183485/
 * @param nWidth width
 * @param nHeight height
 * @param cBitsPerPixel bits per pixel
 * @param data pixel buffer
 * @return new bitmap
 */

HBITMAP CreateBitmap(int nWidth, int nHeight, int cBitsPerPixel, char* data)
{
	HBITMAP hBitmap = (HBITMAP) malloc(sizeof(BITMAP));
	hBitmap->objectType = GDIOBJ_BITMAP;
	hBitmap->bitsPerPixel = cBitsPerPixel;
	hBitmap->bytesPerPixel = (cBitsPerPixel + 1) / 8;
	hBitmap->scanline = nWidth * hBitmap->bytesPerPixel;
	hBitmap->width = nWidth;
	hBitmap->height = nHeight;
	hBitmap->data = (char*) data;
	return hBitmap;
}

/**
 * Create a new bitmap of the given width and height compatible with the current device context.
 * http://msdn.microsoft.com/en-us/library/dd183488/
 * @param hdc device context
 * @param nWidth width
 * @param nHeight height
 * @return new bitmap
 */

HBITMAP CreateCompatibleBitmap(HDC hdc, int nWidth, int nHeight)
{
	HBITMAP hBitmap = (HBITMAP) malloc(sizeof(BITMAP));
	hBitmap->objectType = GDIOBJ_BITMAP;
	hBitmap->bytesPerPixel = hdc->bytesPerPixel;
	hBitmap->bitsPerPixel = hdc->bitsPerPixel;
	hBitmap->width = nWidth;
	hBitmap->height = nHeight;
	hBitmap->data = malloc(nWidth * nHeight * hBitmap->bytesPerPixel);
	hBitmap->scanline = nWidth * hBitmap->bytesPerPixel;
	return hBitmap;
}

/**
 * Compare two bitmaps for equality.
 * @param hBmp1 first bitmap
 * @param hBmp2 second bitmap
 * @return 1 if both bitmaps are equal, 0 otherwise
 */

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

		if (hBmp1->bytesPerPixel == 32)
		{
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
		else if (hBmp1->bytesPerPixel == 16)
		{
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
				}
			}
		}
	}
	else
	{
		return 0;
	}

	return 1;
}

/**
 * Create a new pen
 * http://msdn.microsoft.com/en-us/library/dd183509/.
 * @param fnPenStyle pen style
 * @param nWidth pen width
 * @param crColor pen color
 * @return new pen
 */

HPEN CreatePen(int fnPenStyle, int nWidth, int crColor)
{
	HPEN hPen = (HPEN) malloc(sizeof(PEN));
	hPen->objectType = GDIOBJ_PEN;
	hPen->style = fnPenStyle;
	hPen->color = crColor;
	hPen->width = nWidth;
	return hPen;
}

/**
 * Create a new palette.
 * http://msdn.microsoft.com/en-us/library/dd183507/
 * @param lplgpl logical palette
 * @return new palette
 */

HPALETTE CreatePalette(LOGPALETTE *lplgpl)
{
	HPALETTE hPalette = (HPALETTE) malloc(sizeof(PALETTE));
	hPalette->objectType = GDIOBJ_PALETTE;
	hPalette->logicalPalette = lplgpl;
	return hPalette;
}

/**
 * Create a new solid brush.
 * http://msdn.microsoft.com/en-us/library/dd183518/
 * @param crColor brush color
 * @return new brush
 */

HBRUSH CreateSolidBrush(COLORREF crColor)
{
	HBRUSH hBrush = (HBRUSH) malloc(sizeof(BRUSH));
	hBrush->objectType = GDIOBJ_BRUSH;
	hBrush->style = BS_SOLID;
	hBrush->color = crColor;
	return hBrush;
}

/**
 * Create a new pattern brush.
 * http://msdn.microsoft.com/en-us/library/dd183508/
 * @param hbmp pattern bitmap
 * @return new brush
 */

HBRUSH CreatePatternBrush(HBITMAP hbmp)
{
	HBRUSH hBrush = (HBRUSH) malloc(sizeof(BRUSH));
	hBrush->objectType = GDIOBJ_BRUSH;
	hBrush->style = BS_PATTERN;
	hBrush->pattern = hbmp;
	return hBrush;
}

/**
 * Create a region from rectangular coordinates.
 * http://msdn.microsoft.com/en-us/library/dd183514/
 * @param nLeftRect x1
 * @param nTopRect y1
 * @param nRightRect x2
 * @param nBottomRect y2
 * @return new region
 */

HRGN CreateRectRgn(int nLeftRect, int nTopRect, int nRightRect, int nBottomRect)
{
	HRGN hRgn = (HRGN) malloc(sizeof(RGN));
	hRgn->objectType = GDIOBJ_REGION;
	hRgn->x = nLeftRect;
	hRgn->y = nTopRect;
	hRgn->w = nRightRect - nLeftRect + 1;
	hRgn->h = nBottomRect - nTopRect + 1;
	hRgn->null = 0;
	return hRgn;
}

/**
 * Create a new rectangle.
 * @param xLeft x1
 * @param yTop y1
 * @param xRight x2
 * @param yBottom y2
 * @return new rectangle
 */

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

/**
 * Convert a rectangle to a region.
 * @param rect source rectangle
 * @param rgn destination region
 */

void RectToRgn(HRECT rect, HRGN rgn)
{
	rgn->x = rect->left;
	rgn->y = rect->top;
	rgn->w = rect->right - rect->left + 1;
	rgn->h = rect->bottom - rect->top + 1;
}

/**
 * Convert rectangular coordinates to a region.
 * @param left x1
 * @param top y1
 * @param right x2
 * @param bottom y2
 * @param rgn destination region
 */

void CRectToRgn(int left, int top, int right, int bottom, HRGN rgn)
{
	rgn->x = left;
	rgn->y = top;
	rgn->w = right - left + 1;
	rgn->h = bottom - top + 1;
}

/**
 * Convert a rectangle to region coordinates.
 * @param rect source rectangle
 * @param x x1
 * @param y y1
 * @param w width
 * @param h height
 */

void RectToCRgn(HRECT rect, int *x, int *y, int *w, int *h)
{
	*x = rect->left;
	*y = rect->top;
	*w = rect->right - rect->left + 1;
	*h = rect->bottom - rect->top + 1;
}

/**
 * Convert rectangular coordinates to region coordinates.
 * @param left x1
 * @param top y1
 * @param right x2
 * @param bottom y2
 * @param x x1
 * @param y y1
 * @param w width
 * @param h height
 */

void CRectToCRgn(int left, int top, int right, int bottom, int *x, int *y, int *w, int *h)
{
	*x = left;
	*y = top;
	*w = right - left + 1;
	*h = bottom - top + 1;
}

/**
 * Convert a region to a rectangle.
 * @param rgn source region
 * @param rect destination rectangle
 */

void RgnToRect(HRGN rgn, HRECT rect)
{
	rect->left = rgn->x;
	rect->top = rgn->y;
	rect->right = rgn->x + rgn->w - 1;
	rect->bottom = rgn->y + rgn->h - 1;
}

/**
 * Convert region coordinates to a rectangle.
 * @param x x1
 * @param y y1
 * @param w width
 * @param h height
 * @param rect destination rectangle
 */

void CRgnToRect(int x, int y, int w, int h, HRECT rect)
{
	rect->left = x;
	rect->top = y;
	rect->right = x + w - 1;
	rect->bottom = y + h - 1;
}

/**
 * Convert a region to rectangular coordinates.
 * @param rgn source region
 * @param left x1
 * @param top y1
 * @param right x2
 * @param bottom y2
 */

void RgnToCRect(HRGN rgn, int *left, int *top, int *right, int *bottom)
{
	*left = rgn->x;
	*top = rgn->y;
	*right = rgn->x + rgn->w - 1;
	*bottom = rgn->y + rgn->h - 1;
}

/**
 * Convert region coordinates to rectangular coordinates.
 * @param x x1
 * @param y y1
 * @param w width
 * @param h height
 * @param left x1
 * @param top y1
 * @param right x2
 * @param bottom y2
 */

void CRgnToCRect(int x, int y, int w, int h, int *left, int *top, int *right, int *bottom)
{
	*left = x;
	*top = y;
	*right = x + w - 1;
	*bottom = y + h - 1;
}

/**
 * Check if copying would involve overlapping regions
 * @param x x1
 * @param y y1
 * @param width width
 * @param height height
 * @param srcx source x1
 * @param srcy source y1
 * @return 1 if there is an overlap, 0 otherwise
 */

int CopyOverlap(int x, int y, int width, int height, int srcx, int srcy)
{
	RECT dst;
	RECT src;

	CRgnToRect(x, y, width, height, &dst);
	CRgnToRect(srcx, srcy, width, height, &src);

	return (dst.right > src.left && dst.left < src.right &&
		dst.bottom > src.top && dst.top < src.bottom) ? 1 : 0;
}

/**
 * Set current foreground draw mode.
 * http://msdn.microsoft.com/en-us/library/dd145088/
 * @param hdc device context
 * @param fnDrawMode draw mode
 * @return previous draw mode
 */

int SetROP2(HDC hdc, int fnDrawMode)
{
	int prevDrawMode = hdc->drawMode;
	hdc->drawMode = fnDrawMode;
	return prevDrawMode;
}

/* http://www.cs.toronto.edu/~smalik/418/tutorial2_bresenham.pdf */

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

/**
 * Draw a line from the current position to the given position.
 * http://msdn.microsoft.com/en-us/library/dd145029/
 * @param hdc device context
 * @param nXEnd ending x position
 * @param nYEnd ending y position
 * @return 1 if successful, 0 otherwise
 */

int LineTo(HDC hdc, int nXEnd, int nYEnd)
{	
	printf("LineTo: posX:%d posY:%d nXEnd:%d nYEnd:%d\n",
	       hdc->pen->posX, hdc->pen->posY, nXEnd, nYEnd);

	/*
	 * According to this MSDN article, LineTo uses a modified version of Bresenham:
	 * http://msdn.microsoft.com/en-us/library/dd145027/
	 *
	 * However, since I couldn't find the specifications of this modified algorithm,
	 * we're going to use the original Bresenham line drawing algorithm for now
	 */

	bresenham(hdc, hdc->pen->posX, hdc->pen->posY, nXEnd, nYEnd);
	
	return 1;
}

/**
 * Move pen from the current device context to a new position.
 * @param hdc device context
 * @param X x position
 * @param Y y position
 * @return 1 if successful, 0 otherwise
 */

int MoveTo(HDC hdc, int X, int Y)
{
	hdc->pen->posX = X;
	hdc->pen->posY = Y;
	return 1;
}

/**
 * Set the coordinates of a given rectangle.
 * http://msdn.microsoft.com/en-us/library/dd145085/
 * @param rc rectangle
 * @param xLeft x1
 * @param yTop y1
 * @param xRight x2
 * @param yBottom y2
 * @return 1 if successful, 0 otherwise
 */

int SetRect(HRECT rc, int xLeft, int yTop, int xRight, int yBottom)
{
	rc->left = xLeft;
	rc->top = yTop;
	rc->right = xRight;
	rc->bottom = yBottom;
	return 1;
}

/**
 * Set the coordinates of a given region.
 * @param hRgn region
 * @param nXLeft x1
 * @param nYLeft y1
 * @param nWidth width
 * @param nHeight height
 * @return
 */

int SetRgn(HRGN hRgn, int nXLeft, int nYLeft, int nWidth, int nHeight)
{
	hRgn->x = nXLeft;
	hRgn->y = nYLeft;
	hRgn->w = nWidth;
	hRgn->h = nHeight;
	hRgn->null = 0;
	return 0;
}

/**
 * Convert rectangular coordinates to a region
 * @param hRgn destination region
 * @param nLeftRect x1
 * @param nTopRect y1
 * @param nRightRect x2
 * @param nBottomRect y2
 * @return
 */

int SetRectRgn(HRGN hRgn, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect)
{
	CRectToRgn(nLeftRect, nTopRect, nRightRect, nBottomRect, hRgn);
	hRgn->null = 0;
	return 0;
}

/**
 * Set the current clipping region coordinates.
 * @param hdc device context
 * @param nXLeft x1
 * @param nYLeft y1
 * @param nWidth width
 * @param nHeight height
 * @return
 */

int SetClipRgn(HDC hdc, int nXLeft, int nYLeft, int nWidth, int nHeight)
{
	return SetRgn(hdc->clip, nXLeft, nYLeft, nWidth, nHeight);
}

/**
 * Get the current clipping region.
 * http://msdn.microsoft.com/en-us/library/dd144866/
 * @param hdc device context
 * @return clipping region
 */

HRGN GetClipRgn(HDC hdc)
{
	return hdc->clip;
}

/**
 * Set the current clipping region to null.
 * @param hdc device context
 * @return
 */

int SetNullClipRgn(HDC hdc)
{
	SetClipRgn(hdc, 0, 0, 0, 0);
	hdc->clip->null = 1;
	return 0;
}

/**
 * Clip coordinates according to clipping region
 * @param hdc device context
 * @param x x1
 * @param y y1
 * @param w width
 * @param h height
 * @param srcx source x1
 * @param srcy source y1
 * @return 1 if there is something to draw, 0 otherwise
 */

int ClipCoords(HDC hdc, int *x, int *y, int *w, int *h, int *srcx, int *srcy)
{
	RECT bmp;
	RECT clip;
	RECT coords;
	HBITMAP hBmp;

	int dx = 0;
	int dy = 0;
	int draw = 1;
	
	if (hdc == NULL)
		return 0;

	hBmp = (HBITMAP) hdc->selectedObject;

	if (hBmp != NULL)
	{
		if (hdc->clip->null)
		{
			CRgnToRect(0, 0, hBmp->width, hBmp->height, &clip);
		}
		else
		{
			RgnToRect(hdc->clip, &clip);
			CRgnToRect(0, 0, hBmp->width, hBmp->height, &bmp);

			if (clip.left < bmp.left)
				clip.left = bmp.left;
		
			if (clip.right > bmp.right)
				clip.right = bmp.right;
		
			if (clip.top < bmp.top)
				clip.top = bmp.top;

			if (clip.bottom > bmp.bottom)
				clip.bottom = bmp.bottom;
		}
	}
	else
	{
		RgnToRect(hdc->clip, &clip);
	}

	CRgnToRect(*x, *y, *w, *h, &coords);

	if (coords.right >= clip.left && coords.left <= clip.right &&
		coords.bottom >= clip.top && coords.top <= clip.bottom)
	{
		/* coordinates overlap with clipping region */

		if (coords.left < clip.left)
		{
			dx = (clip.left - coords.left) + 1;
			coords.left = clip.left;
		}

		if (coords.right > clip.right)
			coords.right = clip.right;

		if (coords.top < clip.top)
		{
			dy = (clip.top - coords.top) + 1;
			coords.top = clip.top;
		}

		if (coords.bottom > clip.bottom)
			coords.bottom = clip.bottom;
	}
	else
	{
		/* coordinates do not overlap with clipping region */
		
		coords.left = 0;
		coords.right = 0;
		coords.top = 0;
		coords.bottom = 0;
		draw = 0;
	}

	if (srcx != NULL)
	{
		if (dx > 0)
		{
			*srcx += dx - 1;
		}
	}

	if (srcy != NULL)
	{
		if (dy > 0)
		{
			*srcy += dy - 1;
		}
	}

	RectToCRgn(&coords, x, y, w, h);
	
	return draw;
}

/**
 * Invalidate a given region, such that it is redrawn on the next region update.
 * http://msdn.microsoft.com/en-us/library/dd145003/
 * @param hdc device context
 * @param x x1
 * @param y y1
 * @param w width
 * @param h height
 * @return
 */

int InvalidateRegion(HDC hdc, int x, int y, int w, int h)
{
	RECT inv;
	RECT rgn;
	HRGN invalid;
	HBITMAP bmp;

	if (hdc->hwnd == NULL)
		return 0;

	if (hdc->hwnd->invalid == NULL)
		return 0;

	invalid = hdc->hwnd->invalid;
	bmp = (HBITMAP) hdc->selectedObject;

	if (invalid->null)
	{
		invalid->x = x;
		invalid->y = y;
		invalid->w = w;
		invalid->h = h;
		invalid->null = 0;
		return 0;
	}

	CRgnToRect(x, y, w, h, &rgn);
	RgnToRect(invalid, &inv);

	if (rgn.left < 0)
		rgn.left = 0;

	if (rgn.top < 0)
		rgn.top = 0;

	if (rgn.left < inv.left)
		inv.left = rgn.left;

	if (rgn.top < inv.top)
		inv.top = rgn.top;

	if (rgn.right > inv.right)
		inv.right = rgn.right;

	if (rgn.bottom > inv.bottom)
		inv.bottom = rgn.bottom;

	RectToRgn(&inv, invalid);
	
	return 0;
}

/**
 * Compare two regions for equality.
 * http://msdn.microsoft.com/en-us/library/dd162700/
 * @param hSrcRgn1 first region
 * @param hSrcRgn2 second region
 * @return 1 if both regions are equal, 0 otherwise
 */

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

/**
 * Copy coordinates from a rectangle to another rectangle
 * @param dst destination rectangle
 * @param src source rectangle
 * @return 1 if successful, 0 otherwise
 */

int CopyRect(HRECT dst, HRECT src)
{
	dst->left = src->left;
	dst->top = src->top;
	dst->right = src->right;
	dst->bottom = src->bottom;
	return 1;
}

/**
 * Check if a point is inside a rectangle.
 * http://msdn.microsoft.com/en-us/library/dd162882/
 * @param rc rectangle
 * @param x point x position
 * @param y point y position
 * @return 1 if the point is inside, 0 otherwise
 */

int PtInRect(HRECT rc, int x, int y)
{
	/* 
	 * points on the left and top sides are considered in,
	 * while points on the right and bottom sides are considered out
	 */
	
	if (x >= rc->left && x <= rc->right)
	{
		if (y >= rc->top && y <= rc->bottom)
		{
			return 1;
		}
	}
	
	return 0;
}

/**
 * Get the current background color.
 * http://msdn.microsoft.com/en-us/library/dd144852/
 * @param hdc device context
 * @return background color
 */

COLORREF GetBkColor(HDC hdc)
{
	return hdc->bkColor;
}

/**
 * Set the current background color.
 * http://msdn.microsoft.com/en-us/library/dd162964/
 * @param hdc device color
 * @param crColor new background color
 * @return previous background color
 */

COLORREF SetBkColor(HDC hdc, COLORREF crColor)
{
	COLORREF previousBkColor = hdc->bkColor;
	hdc->bkColor = crColor;
	return previousBkColor;
}

/**
 * Set the current text color.
 * http://msdn.microsoft.com/en-us/library/dd145093/
 * @param hdc device context
 * @param crColor new text color
 * @return previous text color
 */

COLORREF SetTextColor(HDC hdc, COLORREF crColor)
{
	COLORREF previousTextColor = hdc->textColor;
	hdc->textColor = crColor;
	return previousTextColor;
}

/**
 * Set the current background mode.
 * http://msdn.microsoft.com/en-us/library/dd162965/
 * @param hdc device context
 * @param iBkMode background mode
 * @return
 */

int SetBkMode(HDC hdc, int iBkMode)
{
	if (iBkMode == OPAQUE || iBkMode == TRANSPARENT)
		hdc->bkMode = iBkMode;
	else
		hdc->bkMode = OPAQUE;
	
	return 0;
}

/**
 * Get pixel at the given coordinates.
 * http://msdn.microsoft.com/en-us/library/dd144909/
 * @param hdc device context
 * @param nXPos pixel x position
 * @param nYPos pixel y position
 * @return pixel color
 */

COLORREF GetPixel(HDC hdc, int nXPos, int nYPos)
{
	HBITMAP hBmp = (HBITMAP) hdc->selectedObject;
	COLORREF* colorp = (COLORREF*)&(hBmp->data[(nYPos * hBmp->width * hdc->bytesPerPixel) + nXPos * hdc->bytesPerPixel]);
	return (COLORREF) *colorp;
}

/**
 * Set pixel at the given coordinates.
 * http://msdn.microsoft.com/en-us/library/dd145078/
 * @param hdc device context
 * @param X pixel x position
 * @param Y pixel y position
 * @param crColor new pixel color
 * @return
 */

COLORREF SetPixel(HDC hdc, int X, int Y, COLORREF crColor)
{
	HBITMAP hBmp = (HBITMAP) hdc->selectedObject;
	*((COLORREF*)&(hBmp->data[(Y * hBmp->width * hdc->bytesPerPixel) + X * hdc->bytesPerPixel])) = crColor;
	return 0;
}

/**
 * Fill a rectangle with the given brush.
 * http://msdn.microsoft.com/en-us/library/dd162719/
 * @param hdc device context
 * @param rect rectangle
 * @param hbr brush
 * @return 1 if successful, 0 otherwise
 */

int FillRect(HDC hdc, HRECT rect, HBRUSH hbr)
{
	switch (hdc->bitsPerPixel)
	{
		case 32:
			return FillRect_32bpp(hdc, rect, hbr);

		case 16:
			return FillRect_16bpp(hdc, rect, hbr);
			
		default:
			return 0;
	}
}

/**
 * Perform a bit blit operation on the given pixel buffers.
 * http://msdn.microsoft.com/en-us/library/dd183370/
 * @param hdcDest destination device context
 * @param nXDest destination x1
 * @param nYDest destination y1
 * @param nWidth width
 * @param nHeight height
 * @param hdcSrc source device context
 * @param nXSrc source x1
 * @param nYSrc source y1
 * @param rop raster operation code
 * @return 1 if successful, 0 otherwise
 */

int BitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, int rop)
{
	switch (hdcDest->bitsPerPixel)
	{
		case 32:
			return BitBlt_32bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, rop);

		case 16:
			return BitBlt_16bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, rop);
			
		default:
			return 0;
	}
}

/**
 * Perform a pattern blit operation on the given pixel buffer.
 * http://msdn.microsoft.com/en-us/library/dd162778/
 * @param hdc device context
 * @param nXLeft x1
 * @param nYLeft y1
 * @param nWidth width
 * @param nHeight height
 * @param rop raster operation code
 * @return 1 if successful, 0 otherwise
 */

int PatBlt(HDC hdc, int nXLeft, int nYLeft, int nWidth, int nHeight, int rop)
{
	switch (hdc->bitsPerPixel)
	{
		case 32:
			return PatBlt_32bpp(hdc, nXLeft, nYLeft, nWidth, nHeight, rop);

		case 16:
			return PatBlt_16bpp(hdc, nXLeft, nYLeft, nWidth, nHeight, rop);

		default:
			return 0;
	}
}

/**
 * Select a GDI object in the current device context.
 * http://msdn.microsoft.com/en-us/library/dd162957/
 * @param hdc device context
 * @param hgdiobj new selected GDI object
 * @return previous selected GDI object
 */

HGDIOBJ SelectObject(HDC hdc, HGDIOBJ hgdiobj)
{
	if (hgdiobj == NULL)
		return NULL;
	
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

/**
 * Delete a GDI object.
 * http://msdn.microsoft.com/en-us/library/dd183539/
 * @param hgdiobj GDI object
 * @return 1 if successful, 0 otherwise
 */

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

/**
 * Delete device context.
 * http://msdn.microsoft.com/en-us/library/dd183533/
 * @param hdc device context
 * @return 1 if successful, 0 otherwise
 */

int DeleteDC(HDC hdc)
{
	free(hdc);
	return 1;
}
