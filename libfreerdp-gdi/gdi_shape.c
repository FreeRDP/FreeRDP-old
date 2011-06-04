/*
   FreeRDP: A Remote Desktop Protocol client.
   GDI Shape Functions

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

#include "gdi.h"
#include "gdi_8bpp.h"
#include "gdi_16bpp.h"
#include "gdi_32bpp.h"
#include "gdi_bitmap.h"

#include "gdi_shape.h"

pFillRect FillRect_[5] =
{
	NULL,
	FillRect_8bpp,
	FillRect_16bpp,
	NULL,
	FillRect_32bpp
};

static void Ellipse_Bresenham(HDC hdc, int x1, int y1, int x2, int y2)
{
	int i;
	long e, e2;
	long dx, dy;
	int a, b, c;
	int bx1, by1;
	int bx2, by2;

	HBITMAP bmp;
	uint8 pixel8;
	uint16 pixel16;
	uint32 pixel32;
	int bpp = hdc->bitsPerPixel;

	a = (x1 < x2) ? x2 - x1 : x1 - x2;
	b = (y1 < y2) ? y2 - y1 : y1 - y2;
	c = b & 1;

	dx = 4 * (1 - a) * b * b;
	dy = 4 * (c + 1) * a * a;
	e = dx + dy + c * a * a;

	if (x1 > x2)
	{
		x1 = x2;
		x2 += a;
	}

	if (y1 > y2)
		y1 = y2;

	y1 += (b + 1) / 2;
	y2 = y1 - c;

	a *= 8 * a;
	c = 8 * b * b;

	pixel8 = pixel16 = pixel32 = 0;
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

	do
	{
		if (bpp == 32)
		{
			gdi_SetPixel_32bpp(bmp, x2, y1, pixel32);
			gdi_SetPixel_32bpp(bmp, x1, y1, pixel32);
			gdi_SetPixel_32bpp(bmp, x1, y2, pixel32);
			gdi_SetPixel_32bpp(bmp, x2, y2, pixel32);
		}
		else if (bpp == 16)
		{
			gdi_SetPixel_16bpp(bmp, x2, y1, pixel16);
			gdi_SetPixel_16bpp(bmp, x1, y1, pixel16);
			gdi_SetPixel_16bpp(bmp, x1, y2, pixel16);
			gdi_SetPixel_16bpp(bmp, x2, y2, pixel16);
		}
		else if (bpp == 8)
		{
			for (i = x1; i < x2; i++)
			{
				gdi_SetPixel_8bpp(bmp, i, y1, pixel8);
				gdi_SetPixel_8bpp(bmp, i, y2, pixel8);
			}

			for (i = y1; i < y2; i++)
			{
				gdi_SetPixel_8bpp(bmp, x1, i, pixel8);
				gdi_SetPixel_8bpp(bmp, x2, i, pixel8);
			}
		}

		e2 = 2 * e;

		if (e2 >= dx)
		{
			x1++;
			x2--;
			e += dx += c;
		}

		if (e2 <= dy)
		{
			y1++;
			y2--;
			e += dy += a;
		}
	}
	while (x1 <= x2);

	while (y1 - y2 < b)
	{
		if (bpp == 32)
		{
			gdi_SetPixel_32bpp(bmp, x1 - 1, ++y1, pixel32);
			gdi_SetPixel_32bpp(bmp, x1 - 1, --y2, pixel32);
		}
		else if (bpp == 16)
		{
			gdi_SetPixel_16bpp(bmp, x1 - 1, ++y1, pixel16);
			gdi_SetPixel_16bpp(bmp, x1 - 1, --y2, pixel16);
		}
		else if (bpp == 8)
		{
			gdi_SetPixel_8bpp(bmp, x1 - 1, ++y1, pixel8);
			gdi_SetPixel_8bpp(bmp, x1 - 1, --y2, pixel8);
		}
	}
}

/**
 * Draw an ellipse
 * @param hdc device context
 * @param nLeftRect x1
 * @param nTopRect y1
 * @param nRightRect x2
 * @param nBottomRect y2
 * @return
 */
int gdi_Ellipse(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect)
{
	Ellipse_Bresenham(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);
	return 1;
}

/**
 * Fill a rectangle with the given brush.\n
 * @msdn{dd162719}
 * @param hdc device context
 * @param rect rectangle
 * @param hbr brush
 * @return 1 if successful, 0 otherwise
 */

int gdi_FillRect(HDC hdc, HRECT rect, HBRUSH hbr)
{
	pFillRect _FillRect = FillRect_[IBPP(hdc->bitsPerPixel)];

	if (_FillRect != NULL)
		return _FillRect(hdc, rect, hbr);
	else
		return 0;
}

/**
 *
 * @param hdc device context
 * @param lpPoints array of points
 * @param nCount number of points
 * @return
 */
int gdi_Polygon(HDC hdc, POINT *lpPoints, int nCount)
{
	return 1;
}

/**
 * Draw a series of closed polygons
 * @param hdc device context
 * @param lpPoints array of series of points
 * @param lpPolyCounts array of number of points in each series
 * @param nCount count of number of points in lpPolyCounts
 * @return
 */
int gdi_PolyPolygon(HDC hdc, POINT *lpPoints, int *lpPolyCounts, int nCount)
{
	return 1;
}

/**
 * Draw a rectangle
 * @param hdc device context
 * @param nLeftRect x1
 * @param nTopRect y1
 * @param nRightRect x2
 * @param nBottomRect y2
 * @return
 */
int gdi_Rectangle(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect)
{
	return 1;
}
