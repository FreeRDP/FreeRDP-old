/*
   FreeRDP: A Remote Desktop Protocol client.
   GDI Device Context Functions

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

/* Device Context Functions: http://msdn.microsoft.com/en-us/library/dd183554 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <freerdp/freerdp.h>

#include "gdi.h"
#include "gdi_region.h"

#include "gdi_dc.h"

/**
 * Get the current device context (a new one is created each time).\n
 * @msdn{dd144871}
 * @return current device context
 */

HDC gdi_GetDC()
{
	HDC hDC = (HDC) malloc(sizeof(DC));
	hDC->bytesPerPixel = 4;
	hDC->bitsPerPixel = 32;
	hDC->drawMode = R2_BLACK;
	hDC->clip = gdi_CreateRectRgn(0, 0, 0, 0);
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

HDC gdi_CreateCompatibleDC(HDC hdc)
{
	HDC hDC = (HDC) malloc(sizeof(DC));
	hDC->bytesPerPixel = hdc->bytesPerPixel;
	hDC->bitsPerPixel = hdc->bitsPerPixel;
	hDC->drawMode = hdc->drawMode;
	hDC->clip = gdi_CreateRectRgn(0, 0, 0, 0);
	hDC->clip->null = 1;
	hDC->hwnd = NULL;
	hDC->alpha = hdc->alpha;
	hDC->invert = hdc->invert;
	hDC->rgb555 = hdc->rgb555;
	return hDC;
}

/**
 * Select a GDI object in the current device context.\n
 * @msdn{dd162957}
 * @param hdc device context
 * @param hgdiobject new selected GDI object
 * @return previous selected GDI object
 */

HGDIOBJECT gdi_SelectObject(HDC hdc, HGDIOBJECT hgdiobject)
{
	HGDIOBJECT previousSelectedObject = hdc->selectedObject;

	if (hgdiobject == NULL)
		return NULL;

	if (hgdiobject->objectType == GDIOBJ_BITMAP)
	{
		hdc->selectedObject = hgdiobject;
	}
	else if (hgdiobject->objectType == GDIOBJ_PEN)
	{
		previousSelectedObject = (HGDIOBJECT) hdc->pen;
		hdc->pen = (HPEN) hgdiobject;
	}
	else if (hgdiobject->objectType == GDIOBJ_BRUSH)
	{
		previousSelectedObject = (HGDIOBJECT) hdc->brush;
		hdc->brush = (HBRUSH) hgdiobject;
	}
	else if (hgdiobject->objectType == GDIOBJ_REGION)
	{
		hdc->selectedObject = hgdiobject;
	}
	else if (hgdiobject->objectType == GDIOBJ_RECT)
	{
		hdc->selectedObject = hgdiobject;
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
 * @param hgdiobject GDI object
 * @return 1 if successful, 0 otherwise
 */

int gdi_DeleteObject(HGDIOBJECT hgdiobject)
{
	if (hgdiobject == NULL)
		return 0;

	if (hgdiobject->objectType == GDIOBJ_BITMAP)
	{
		HBITMAP hBitmap = (HBITMAP) hgdiobject;

		if (hBitmap->data != NULL)
			free(hBitmap->data);

		free(hBitmap);
	}
	else if (hgdiobject->objectType == GDIOBJ_PEN)
	{
		HPEN hPen = (HPEN) hgdiobject;
		free(hPen);
	}
	else if (hgdiobject->objectType == GDIOBJ_BRUSH)
	{
		HBRUSH hBrush = (HBRUSH) hgdiobject;

		if(hBrush->style == BS_PATTERN)
			gdi_DeleteObject((HGDIOBJECT) hBrush->pattern);

		free(hBrush);
	}
	else if (hgdiobject->objectType == GDIOBJ_REGION)
	{
		free(hgdiobject);
	}
	else if (hgdiobject->objectType == GDIOBJ_RECT)
	{
		free(hgdiobject);
	}
	else
	{
		/* Unknown GDI Object Type */
		free(hgdiobject);
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

int gdi_DeleteDC(HDC hdc)
{
	if (hdc->hwnd)
	{
		free(hdc->hwnd->invalid);
		free(hdc->hwnd);
	}

	free(hdc->clip);
	free(hdc);

	return 1;
}
