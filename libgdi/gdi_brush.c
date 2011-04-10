/*
   FreeRDP: A Remote Desktop Protocol client.
   GDI Brush Functions

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

/* GDI Brush Functions: http://msdn.microsoft.com/en-us/library/dd183395/ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <freerdp/freerdp.h>
#include "gdi.h"

#include "gdi_32bpp.h"
#include "gdi_16bpp.h"
#include "gdi_8bpp.h"

#include "gdi_brush.h"

pPatBlt PatBlt_[5];

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

void BrushInit()
{
	/* PatBlt */
	PatBlt_[1] = PatBlt_8bpp;
	PatBlt_[2] = PatBlt_16bpp;
	PatBlt_[4] = PatBlt_32bpp;
}
