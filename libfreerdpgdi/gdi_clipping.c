/*
   FreeRDP: A Remote Desktop Protocol client.
   GDI Clipping Functions

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

#include "gdi_clipping.h"

int SetClipRgn(HDC hdc, int nXLeft, int nYLeft, int nWidth, int nHeight)
{
	return SetRgn(hdc->clip, nXLeft, nYLeft, nWidth, nHeight);
}

/**
 * Get the current clipping region.\n
 * @msdn{dd144866}
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
