/*
   FreeRDP: A Remote Desktop Protocol client.
   GDI Bitmap Functions

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <freerdp/freerdp.h>
#include "libfreerdpgdi.h"

#include "gdi_color.h"
#include "gdi_window.h"
#include "gdi_32bpp.h"
#include "gdi_16bpp.h"
#include "gdi_8bpp.h"

#include "gdi_bitmap.h"

pBitBlt BitBlt_[5];
pPatBlt PatBlt_[5];
pFillRect FillRect_[5];

/**
 * Get pixel at the given coordinates.\n
 * @msdn{dd144909}
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
 * Set pixel at the given coordinates.\n
 * @msdn{dd145078}
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

void SetPixel_8bpp(HBITMAP hBmp, int X, int Y, uint8 pixel)
{
	*((uint8*)&(hBmp->data[(Y * hBmp->width) + X])) = pixel;
}

void SetPixel_16bpp(HBITMAP hBmp, int X, int Y, uint16 pixel)
{
	*((uint16*)&(hBmp->data[(Y * hBmp->width * 2) + X * 2])) = pixel;
}

void SetPixel_32bpp(HBITMAP hBmp, int X, int Y, uint32 pixel)
{
	*((uint32*)&(hBmp->data[(Y * hBmp->width * 4) + X * 4])) = pixel;
}

/**
 * Create a new bitmap with the given width, height, color format and pixel buffer.\n
 * @msdn{dd183485}
 * @param nWidth width
 * @param nHeight height
 * @param cBitsPerPixel bits per pixel
 * @param data pixel buffer
 * @return new bitmap
 */

HBITMAP CreateBitmap(int nWidth, int nHeight, int cBitsPerPixel, uint8* data)
{
	HBITMAP hBitmap = (HBITMAP) malloc(sizeof(BITMAP));
	hBitmap->objectType = GDIOBJ_BITMAP;
	hBitmap->bitsPerPixel = cBitsPerPixel;
	hBitmap->bytesPerPixel = (cBitsPerPixel + 1) / 8;
	hBitmap->scanline = nWidth * hBitmap->bytesPerPixel;
	hBitmap->width = nWidth;
	hBitmap->height = nHeight;
	hBitmap->data = data;
	return hBitmap;
}

/**
 * Create a new bitmap of the given width and height compatible with the current device context.\n
 * @msdn{dd183488}
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
 * Fill a rectangle with the given brush.\n
 * @msdn{dd162719}
 * @param hdc device context
 * @param rect rectangle
 * @param hbr brush
 * @return 1 if successful, 0 otherwise
 */

int FillRect(HDC hdc, HRECT rect, HBRUSH hbr)
{
	return FillRect_[IBPP(hdc->bitsPerPixel)](hdc, rect, hbr);
}

/**
 * Perform a bit blit operation on the given pixel buffers.\n
 * @msdn{dd183370}
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
	return BitBlt_[IBPP(hdcDest->bitsPerPixel)](hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, rop);
}

/**
 * Perform a pattern blit operation on the given pixel buffer.\n
 * @msdn{dd162778}
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
	return PatBlt_[IBPP(hdc->bitsPerPixel)](hdc, nXLeft, nYLeft, nWidth, nHeight, rop);
}

void GDIBitmapInit()
{
	/* BitBlt */
	BitBlt_[1] = BitBlt_8bpp;
	BitBlt_[2] = BitBlt_16bpp;
	BitBlt_[4] = BitBlt_32bpp;

	/* PatBlt */
	PatBlt_[1] = PatBlt_8bpp;
	PatBlt_[2] = PatBlt_16bpp;
	PatBlt_[4] = PatBlt_32bpp;

	/* FillRect */
	FillRect_[1] = FillRect_8bpp;
	FillRect_[2] = FillRect_16bpp;
	FillRect_[4] = FillRect_32bpp;
}
