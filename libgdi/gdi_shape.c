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

#include "gdi_32bpp.h"
#include "gdi_16bpp.h"
#include "gdi_8bpp.h"

#include "gdi_shape.h"

pFillRect FillRect_[5];

/**
 * Draw an ellipse
 * @param hdc device context
 * @param nLeftRect x1
 * @param nTopRect y1
 * @param nRightRect x2
 * @param nBottomRect y2
 * @return
 */
int Ellipse(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect)
{
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

int FillRect(HDC hdc, HRECT rect, HBRUSH hbr)
{
	return FillRect_[IBPP(hdc->bitsPerPixel)](hdc, rect, hbr);
}

/**
 *
 * @param hdc device context
 * @param lpPoints array of points
 * @param nCount number of points
 * @return
 */
int Polygon(HDC hdc, POINT *lpPoints, int nCount)
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
int PolyPolygon(HDC hdc, POINT *lpPoints, int *lpPolyCounts, int nCount)
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
int Rectangle(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect)
{
	return 1;
}

void ShapeInit()
{
	/* FillRect */
	FillRect_[1] = FillRect_8bpp;
	FillRect_[2] = FillRect_16bpp;
	FillRect_[4] = FillRect_32bpp;
}
