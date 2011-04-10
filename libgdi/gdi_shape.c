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

int Ellipse(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect)
{
	return 1;
}

int Polygon(HDC hdc, POINT *lpPoints, int nCount)
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

void ShapeInit()
{
	/* FillRect */
	FillRect_[1] = FillRect_8bpp;
	FillRect_[2] = FillRect_16bpp;
	FillRect_[4] = FillRect_32bpp;
}
