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
#include "gdi_8bpp.h"

/**
 * Get the current device context (a new one is created each time).\n
 * @msdn{dd144871}
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
 * Create a new device context compatible with the given device context.\n
 * @msdn{dd183489}
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
 * Create a new pen.\n
 * @msdn{dd183509}
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
 * Create a new palette.\n
 * @msdn{dd183507}
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
 * Create a new solid brush.\n
 * @msdn{dd183518}
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
 * Create a new pattern brush.\n
 * @msdn{dd183508}
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
 * Set current foreground draw mode.\n
 * @msdn{dd145088}
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

static HPALETTE hSystemPalette = NULL;

static const PALETTEENTRY default_system_palette[20] =
{
	/* First 10 entries */
	{ 0x00, 0x00, 0x00 },
	{ 0x80, 0x00, 0x00 },
	{ 0x00, 0x80, 0x00 },
	{ 0x80, 0x80, 0x00 },
	{ 0x00, 0x00, 0x80 },
	{ 0x80, 0x00, 0x80 },
	{ 0x00, 0x80, 0x80 },
	{ 0xC0, 0xC0, 0xC0 },
	{ 0xC0, 0xDC, 0xC0 },
	{ 0xA6, 0xCA, 0xF0 },

	/* Last 10 entries */
	{ 0xFF, 0xFB, 0xF0 },
	{ 0xA0, 0xA0, 0xA4 },
	{ 0x80, 0x80, 0x80 },
	{ 0xFF, 0x00, 0x00 },
	{ 0x00, 0xFF, 0x00 },
	{ 0xFF, 0xFF, 0x00 },
	{ 0x00, 0x00, 0xFF },
	{ 0xFF, 0x00, 0xFF },
	{ 0x00, 0xFF, 0xFF },
	{ 0xFF, 0xFF, 0xFF }
};

/**
 * Create system palette\n
 * @return system palette
 */

HPALETTE CreateSystemPalette()
{
	HPALETTE hPalette;
	LOGPALETTE *logicalPalette;

	logicalPalette = (LOGPALETTE*) malloc(sizeof(LOGPALETTE));
	logicalPalette->count = 256;
	logicalPalette->entries = (PALETTEENTRY*) malloc(sizeof(PALETTEENTRY) * 256);
	memset(logicalPalette->entries, 0, sizeof(PALETTEENTRY) * 256);

	memcpy(&logicalPalette->entries[0], &default_system_palette[0], 10 * sizeof(PALETTEENTRY));
	memcpy(&logicalPalette->entries[256 - 10], &default_system_palette[10], 10 * sizeof(PALETTEENTRY));

	hPalette = CreatePalette(logicalPalette);

	return hPalette;
}

/**
 * Get system palette\n
 * @return system palette
 */

HPALETTE GetSystemPalette()
{
	if (hSystemPalette == NULL)
		hSystemPalette = CreateSystemPalette();

	return hSystemPalette;
}

/**
 * Invalidate a given region, such that it is redrawn on the next region update.\n
 * @msdn{dd145003}
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
 * Get the current background color.\n
 * @msdn{dd144852}
 * @param hdc device context
 * @return background color
 */

COLORREF GetBkColor(HDC hdc)
{
	return hdc->bkColor;
}

/**
 * Set the current background color.\n
 * @msdn{dd162964}
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
 * Set the current text color.\n
 * @msdn{dd145093}
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
 * Set the current background mode.\n
 * @msdn{dd162965}
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
 * Select a GDI object in the current device context.\n
 * @msdn{dd162957}
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
 * Delete a GDI object.\n
 * @msdn{dd183539}
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
 * Delete device context.\n
 * @msdn{dd183533}
 * @param hdc device context
 * @return 1 if successful, 0 otherwise
 */

int DeleteDC(HDC hdc)
{
	free(hdc);
	return 1;
}

void InitializeGDI()
{
	GDIBitmapInit();
}
