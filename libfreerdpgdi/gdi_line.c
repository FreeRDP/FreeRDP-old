/*
   FreeRDP: A Remote Desktop Protocol client.
   GDI Line Functions

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
#include "gdi_bitmap.h"
#include "gdi_32bpp.h"
#include "gdi_16bpp.h"
#include "gdi_8bpp.h"

#include "gdi_line.h"

static void
LineTo_Bresenham_8bpp(HDC hdc, int x1, int y1, int x2, int y2)
{
	int x, y;
	int e, e2;
	int dx, dy;
	int sx, sy;
	uint8 pixel;
	HBITMAP bmp;
	int bx1, by1;
	int bx2, by2;

	dx = (x1 > x2) ? x1 - x2 : x2 - x1;
	dy = (y1 > y2) ? y1 - y2 : y2 - y1;

	sx = (x1 < x2) ? 1 : -1;
	sy = (y1 < y2) ? 1 : -1;

	e = dx - dy;

	x = x1;
	y = y1;

	pixel = 0;
	bmp = (HBITMAP) hdc->selectedObject;

	if (hdc->clip->null)
	{
		bx1 = (x1 < x2) ? x1 : x2;
		by1 = (y1 < y2) ? y1 : y2;
		bx2 = (x1 > x2) ? x1 : x2;
		by2 = (y1 > y2) ? y1 : y2;
	}
	else
	{
		bx1 = hdc->clip->x;
		by1 = hdc->clip->y;
		bx2 = bx1 + hdc->clip->w - 1;
		by2 = by1 + hdc->clip->h - 1;
	}

	while (1)
	{
		if (!(x == x2 && y == y2))
		{
			if ((x >= bx1 && x <= bx2) && (y >= by1 && y <= by2))
				SetPixel_8bpp(bmp, x, y, pixel);
		}
		else
		{
			break;
		}

		e2 = 2 * e;

		if (e2 > -dy)
		{
			e -= dy;
			x += sx;
		}

		if (e2 < dx)
		{
			e += dx;
			y += sy;
		}
	}
}

static void
LineTo_Bresenham_16bpp(HDC hdc, int x1, int y1, int x2, int y2)
{
	int x, y;
	int e, e2;
	int dx, dy;
	int sx, sy;
	uint16 pixel;
	HBITMAP bmp;
	int bx1, by1;
	int bx2, by2;

	dx = (x1 > x2) ? x1 - x2 : x2 - x1;
	dy = (y1 > y2) ? y1 - y2 : y2 - y1;

	sx = (x1 < x2) ? 1 : -1;
	sy = (y1 < y2) ? 1 : -1;

	e = dx - dy;

	x = x1;
	y = y1;

	pixel = 0;
	bmp = (HBITMAP) hdc->selectedObject;

	if (hdc->clip->null)
	{
		bx1 = (x1 < x2) ? x1 : x2;
		by1 = (y1 < y2) ? y1 : y2;
		bx2 = (x1 > x2) ? x1 : x2;
		by2 = (y1 > y2) ? y1 : y2;
	}
	else
	{
		bx1 = hdc->clip->x;
		by1 = hdc->clip->y;
		bx2 = bx1 + hdc->clip->w - 1;
		by2 = by1 + hdc->clip->h - 1;
	}

	while (1)
	{
		if (!(x == x2 && y == y2))
		{
			if ((x >= bx1 && x <= bx2) && (y >= by1 && y <= by2))
				SetPixel_16bpp(bmp, x, y, pixel);
		}
		else
		{
			break;
		}

		e2 = 2 * e;

		if (e2 > -dy)
		{
			e -= dy;
			x += sx;
		}

		if (e2 < dx)
		{
			e += dx;
			y += sy;
		}
	}
}

static void
LineTo_Bresenham_32bpp(HDC hdc, int x1, int y1, int x2, int y2)
{
	int x, y;
	int e, e2;
	int dx, dy;
	int sx, sy;
	uint32 pixel;
	HBITMAP bmp;
	int bx1, by1;
	int bx2, by2;

	dx = (x1 > x2) ? x1 - x2 : x2 - x1;
	dy = (y1 > y2) ? y1 - y2 : y2 - y1;

	sx = (x1 < x2) ? 1 : -1;
	sy = (y1 < y2) ? 1 : -1;

	e = dx - dy;

	x = x1;
	y = y1;

	pixel = 0;
	bmp = (HBITMAP) hdc->selectedObject;

	if (hdc->clip->null)
	{
		bx1 = (x1 < x2) ? x1 : x2;
		by1 = (y1 < y2) ? y1 : y2;
		bx2 = (x1 > x2) ? x1 : x2;
		by2 = (y1 > y2) ? y1 : y2;
	}
	else
	{
		bx1 = hdc->clip->x;
		by1 = hdc->clip->y;
		bx2 = bx1 + hdc->clip->w - 1;
		by2 = by1 + hdc->clip->h - 1;
	}

	while (1)
	{
		if (!(x == x2 && y == y2))
		{
			if ((x >= bx1 && x <= bx2) && (y >= by1 && y <= by2))
				SetPixel_32bpp(bmp, x, y, pixel);
		}
		else
		{
			break;
		}

		e2 = 2 * e;

		if (e2 > -dy)
		{
			e -= dy;
			x += sx;
		}

		if (e2 < dx)
		{
			e += dx;
			y += sy;
		}
	}
}

/**
 * Draw a line from the current position to the given position.\n
 * @msdn{dd145029}
 * @param hdc device context
 * @param nXEnd ending x position
 * @param nYEnd ending y position
 * @return 1 if successful, 0 otherwise
 */

int LineTo(HDC hdc, int nXEnd, int nYEnd)
{
	/*
	 * According to this MSDN article, LineTo uses a modified version of Bresenham:
	 * http://msdn.microsoft.com/en-us/library/dd145027/
	 */

	if (hdc->bitsPerPixel == 32)
		LineTo_Bresenham_32bpp(hdc, hdc->pen->posX, hdc->pen->posY, nXEnd, nYEnd);
	else if (hdc->bitsPerPixel == 16)
		LineTo_Bresenham_16bpp(hdc, hdc->pen->posX, hdc->pen->posY, nXEnd, nYEnd);
	else if (hdc->bitsPerPixel == 8)
		LineTo_Bresenham_8bpp(hdc, hdc->pen->posX, hdc->pen->posY, nXEnd, nYEnd);

	return 1;
}

/**
 * Move pen from the current device context to a new position.
 * @param hdc device context
 * @param X x position
 * @param Y y position
 * @return 1 if successful, 0 otherwise
 */

int MoveToEx(HDC hdc, int X, int Y, HPOINT lpPoint)
{
	if (lpPoint != NULL)
	{
		lpPoint->x = hdc->pen->posX;
		lpPoint->y = hdc->pen->posY;
	}

	hdc->pen->posX = X;
	hdc->pen->posY = Y;

	return 1;
}
