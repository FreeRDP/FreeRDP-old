/*
   FreeRDP: A Remote Desktop Protocol client.
   16bpp Internal Buffer Routines

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

#include "gdi.h"
#include "color.h"
#include "gdi_pen.h"
#include "gdi_bitmap.h"
#include "gdi_region.h"
#include "gdi_clipping.h"
#include "gdi_drawing.h"

#include "gdi_16bpp.h"

uint16 gdi_get_color_16bpp(HDC hdc, COLORREF color)
{
	uint8 r, g, b;
	uint16 color16;

	GetRGB32(r, g, b, color);

	if(hdc->rgb555)
	{
		if (hdc->invert)
		{
			color16 = BGR15(r, g, b);
		}
		else
		{
			color16 = RGB15(r, g, b);
		}
	}
	else
	{
		if (hdc->invert)
		{
			color16 = BGR16(r, g, b);
		}
		else
		{
			color16 = RGB16(r, g, b);
		}
	}

	return color16;
}

int FillRect_16bpp(HDC hdc, HRECT rect, HBRUSH hbr)
{
	int x, y;
	uint16 *dstp;
	int nXDest, nYDest;
	int nWidth, nHeight;

	uint16 color16;
	
	RectToCRgn(rect, &nXDest, &nYDest, &nWidth, &nHeight);
	
	if (ClipCoords(hdc, &nXDest, &nYDest, &nWidth, &nHeight, NULL, NULL) == 0)
		return 0;

	color16 = gdi_get_color_16bpp(hdc, hbr->color);

	for (y = 0; y < nHeight; y++)
	{
		dstp = (uint16*) gdi_get_bitmap_pointer(hdc, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				*dstp = color16;
				dstp++;
			}
		}
	}

	InvalidateRegion(hdc, nXDest, nYDest, nWidth, nHeight);
	return 0;
}

static int BitBlt_BLACKNESS_16bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight)
{
	int y;
	uint8 *dstp;

	for (y = 0; y < nHeight; y++)
	{
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
			memset(dstp, 0, nWidth * hdcDest->bytesPerPixel);
	}

	return 0;
}

static int BitBlt_WHITENESS_16bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight)
{
	int y;
	uint8 *dstp;
	
	for (y = 0; y < nHeight; y++)
	{
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
			memset(dstp, 0xFF, nWidth * hdcDest->bytesPerPixel);
	}

	return 0;
}

static int BitBlt_SRCCOPY_16bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{
	int y;
	uint8 *srcp;
	uint8 *dstp;

	if ((hdcDest->selectedObject != hdcSrc->selectedObject) ||
	    CopyOverlap(nXDest, nYDest, nWidth, nHeight, nXSrc, nYSrc) == 0)
	{
		for (y = 0; y < nHeight; y++)
		{
			srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc + y);
			dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

			if (srcp != 0 && dstp != 0)
			{
				gdi_copy_mem(dstp, srcp, nWidth * hdcDest->bytesPerPixel);
			}
		}

		return 0;
	}
	
	if (nYSrc < nYDest)
	{
		/* copy down (bottom to top) */
		for (y = nHeight - 1; y >= 0; y--)
		{
			srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc + y);
			dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

			if (srcp != 0 && dstp != 0)
			{
				gdi_copy_mem(dstp, srcp, nWidth * hdcDest->bytesPerPixel);
			}
		}
	}
	else if (nYSrc > nYDest || nXSrc > nXDest)
	{
		/* copy up or left (top top bottom) */
		for (y = 0; y < nHeight; y++)
		{
			srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc + y);
			dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

			if (srcp != 0 && dstp != 0)
			{
				gdi_copy_mem(dstp, srcp, nWidth * hdcDest->bytesPerPixel);
			}
		}
	}
	else
	{
		/* copy straight right */
		for (y = 0; y < nHeight; y++)
		{
			srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc + y);
			dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

			if (srcp != 0 && dstp != 0)
			{
				gdi_copy_memb(dstp, srcp, nWidth * hdcDest->bytesPerPixel);
			}
		}
	}
	
	return 0;
}

static int BitBlt_NOTSRCCOPY_16bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{
	int x, y;
	uint8 *srcp;
	uint8 *dstp;
	
	for (y = 0; y < nHeight; y++)
	{
		srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc + y);
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				*dstp = ~(*srcp);
				srcp++;
				dstp++;
					
				*dstp = ~(*srcp);
				srcp++;
				dstp++;
			}
		}
	}

	return 0;
}

static int BitBlt_DSTINVERT_16bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight)
{
	int x, y;
	uint8 *dstp;
		
	for (y = 0; y < nHeight; y++)
	{
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				*dstp = ~(*dstp);
				dstp++;
					
				*dstp = ~(*dstp);
				dstp++;
			}
		}
	}

	return 0;
}

static int BitBlt_SRCERASE_16bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{
	int x, y;
	uint8 *srcp;
	uint8 *dstp;
		
	for (y = 0; y < nHeight; y++)
	{
		srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc + y);
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				*dstp = *srcp & ~(*dstp);
				srcp++;
				dstp++;
					
				*dstp = *srcp & ~(*dstp);
				srcp++;
				dstp++;
			}
		}
	}

	return 0;
}

static int BitBlt_NOTSRCERASE_16bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{
	int x, y;
	uint8 *srcp;
	uint8 *dstp;
		
	for (y = 0; y < nHeight; y++)
	{
		srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc + y);
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				*dstp = ~(*srcp) & ~(*dstp);
				srcp++;
				dstp++;
					
				*dstp = ~(*srcp) & ~(*dstp);
				srcp++;
				dstp++;
			}
		}
	}

	return 0;
}

static int BitBlt_SRCINVERT_16bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{
	int x, y;
	uint8 *srcp;
	uint8 *dstp;
		
	for (y = 0; y < nHeight; y++)
	{
		srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc + y);
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				*dstp ^= *srcp;
				srcp++;
				dstp++;
					
				*dstp ^= *srcp;
				srcp++;
				dstp++;
			}
		}
	}

	return 0;
}

static int BitBlt_SRCAND_16bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{
	int x, y;
	uint8 *srcp;
	uint8 *dstp;
		
	for (y = 0; y < nHeight; y++)
	{
		srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc + y);
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				*dstp &= *srcp;
				srcp++;
				dstp++;
					
				*dstp &= *srcp;
				srcp++;
				dstp++;
			}
		}
	}

	return 0;
}

static int BitBlt_SRCPAINT_16bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{
	int x, y;
	uint8 *srcp;
	uint8 *dstp;
		
	for (y = 0; y < nHeight; y++)
	{
		srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc + y);
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				*dstp |= *srcp;
				srcp++;
				dstp++;
					
				*dstp |= *srcp;
				srcp++;
				dstp++;
			}
		}
	}

	return 0;
}

static int BitBlt_DSPDxax_16bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{	
	int x, y;
	uint8 *srcp;
	uint8 *dstp;
	uint8 *patp;
	uint16 color16;
	HBITMAP hSrcBmp;

	/* D = (S & P) | (~S & D) */
	/* DSPDxax, used to draw glyphs */

	color16 = gdi_get_color_16bpp(hdcDest, hdcDest->textColor);

	hSrcBmp = (HBITMAP) hdcSrc->selectedObject;
	srcp = hSrcBmp->data;

	if (hdcSrc->bytesPerPixel != 1)
	{
		printf("BitBlt_DSPDxax expects 1 bpp, unimplemented for %d\n", hdcSrc->bytesPerPixel);
		return 0;
	}
	
	for (y = 0; y < nHeight; y++)
	{
		srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc + y);
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				patp = (uint8*) &color16;

				*dstp = (*srcp & *patp) | (~(*srcp) & *dstp);
				dstp++;
				patp++;

				*dstp = (*srcp & *patp) | (~(*srcp) & *dstp);
				dstp++;
				patp++;
				srcp++;
			}
		}
	}

	return 0;
}

static int BitBlt_SPna_16bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{
	int x, y;
	uint8 *srcp;
	uint8 *dstp;
	uint8 *patp;
	
	for (y = 0; y < nHeight; y++)
	{
		srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc + y);
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);		

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				patp = gdi_get_brush_pointer(hdcDest, x, y);
				
				*dstp = *srcp & ~(*patp);
				patp++;
				srcp++;
				dstp++;
					
				*dstp = *srcp & ~(*patp);
				patp++;
				srcp++;
				dstp++;
			}
		}
	}
	
	return 0;
}

static int BitBlt_DSna_16bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{
	int x, y;
	uint8 *srcp;
	uint8 *dstp;

	for (y = 0; y < nHeight; y++)
	{
		srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc + y);
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				*dstp = ~(*srcp) & (*dstp);
				srcp++;
				dstp++;

				*dstp = ~(*srcp) & (*dstp);
				srcp++;
				dstp++;
			}
		}
	}

	return 0;
}


static int BitBlt_MERGECOPY_16bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{
	int x, y;
	uint8 *srcp;
	uint8 *dstp;
	uint8 *patp;
	
	for (y = 0; y < nHeight; y++)
	{
		srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc + y);
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				patp = gdi_get_brush_pointer(hdcDest, x, y);
				
				*dstp = *srcp & *patp;
				patp++;
				srcp++;
				dstp++;
					
				*dstp = *srcp & *patp;
				patp++;
				srcp++;
				dstp++;
			}
		}
	}
	
	return 0;
}

static int BitBlt_MERGEPAINT_16bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{
	int x, y;
	uint8 *srcp;
	uint8 *dstp;
	
	for (y = 0; y < nHeight; y++)
	{
		srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc + y);
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{				
				*dstp = ~(*srcp) | *dstp;
				srcp++;
				dstp++;
					
				*dstp = ~(*srcp) | *dstp;
				srcp++;
				dstp++;
			}
		}
	}
	
	return 0;
}

static int BitBlt_PATCOPY_16bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight)
{
	int x, y;
	uint8 *dstp;
	uint8 *patp;
	uint16 color16;
	uint16 *dstp16;

	if(hdcDest->brush->style == BS_SOLID)
	{
		color16 = gdi_get_color_16bpp(hdcDest, hdcDest->brush->color);

		for (y = 0; y < nHeight; y++)
		{
			dstp16 = (uint16*) gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

			if (dstp16 != 0)
			{
				for (x = 0; x < nWidth; x++)
				{
					*dstp16 = color16;
					dstp16++;
				}
			}
		}
	}
	else
	{
		for (y = 0; y < nHeight; y++)
		{
			dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

			if (dstp != 0)
			{
				for (x = 0; x < nWidth; x++)
				{
					patp = gdi_get_brush_pointer(hdcDest, x, y);
					
					*dstp = *patp;
					patp++;
					dstp++;

					*dstp = *patp;
					patp++;
					dstp++;
				}
			}
		}
	}

	return 0;
}

static int BitBlt_PATINVERT_16bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight)
{
	int x, y;
	uint8 *dstp;
	uint8 *patp;
	uint16 color16;
	uint16 *dstp16;

	if(hdcDest->brush->style == BS_SOLID)
	{
		color16 = gdi_get_color_16bpp(hdcDest, hdcDest->brush->color);

		for (y = 0; y < nHeight; y++)
		{
			dstp16 = (uint16*) gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

			if (dstp16 != 0)
			{
				for (x = 0; x < nWidth; x++)
				{
					*dstp16 ^= color16;
					dstp16++;
				}
			}
		}
	}
	else
	{
		for (y = 0; y < nHeight; y++)
		{
			dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

			if (dstp != 0)
			{
				for (x = 0; x < nWidth; x++)
				{
					patp = gdi_get_brush_pointer(hdcDest, x, y);
					
					*dstp = *patp ^ *dstp;
					patp++;
					dstp++;

					*dstp = *patp ^ *dstp;
					patp++;
					dstp++;
				}
			}
		}
	}
	
	return 0;
}

static int BitBlt_PATPAINT_16bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{
	int x, y;
	uint8 *srcp;
	uint8 *dstp;
	uint8 *patp;
	
	for (y = 0; y < nHeight; y++)
	{
		srcp = gdi_get_bitmap_pointer(hdcSrc, nXSrc, nYSrc + y);
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				patp = gdi_get_brush_pointer(hdcDest, x, y);
				
				*dstp = *dstp | (*patp | ~(*srcp));
				patp++;
				srcp++;
				dstp++;
					
				*dstp = *dstp | (*patp | ~(*srcp));
				patp++;
				srcp++;
				dstp++;
			}
		}
	}
	
	return 0;
}

int BitBlt_16bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, int rop)
{
	if (hdcSrc != NULL)
	{
		if (ClipCoords(hdcDest, &nXDest, &nYDest, &nWidth, &nHeight, &nXSrc, &nYSrc) == 0)
			return 0;
	}
	else
	{
		if (ClipCoords(hdcDest, &nXDest, &nYDest, &nWidth, &nHeight, NULL, NULL) == 0)
			return 0;
	}
	
	InvalidateRegion(hdcDest, nXDest, nYDest, nWidth, nHeight);
	
	switch (rop)
	{
		case BLACKNESS:
			return BitBlt_BLACKNESS_16bpp(hdcDest, nXDest, nYDest, nWidth, nHeight);
			break;

		case WHITENESS:
			return BitBlt_WHITENESS_16bpp(hdcDest, nXDest, nYDest, nWidth, nHeight);
			break;

		case SRCCOPY:
			return BitBlt_SRCCOPY_16bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case SPna:
			return BitBlt_SPna_16bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case DSna:
			return BitBlt_DSna_16bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case DSPDxax:
			return BitBlt_DSPDxax_16bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;
			
		case NOTSRCCOPY:
			return BitBlt_NOTSRCCOPY_16bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case DSTINVERT:
			return BitBlt_DSTINVERT_16bpp(hdcDest, nXDest, nYDest, nWidth, nHeight);
			break;

		case SRCERASE:
			return BitBlt_SRCERASE_16bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case NOTSRCERASE:
			return BitBlt_NOTSRCERASE_16bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case SRCINVERT:
			return BitBlt_SRCINVERT_16bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case SRCAND:
			return BitBlt_SRCAND_16bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case SRCPAINT:
			return BitBlt_SRCPAINT_16bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case MERGECOPY:
			return BitBlt_MERGECOPY_16bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case MERGEPAINT:
			return BitBlt_MERGEPAINT_16bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case PATCOPY:
			return BitBlt_PATCOPY_16bpp(hdcDest, nXDest, nYDest, nWidth, nHeight);
			break;

		case PATINVERT:
			return BitBlt_PATINVERT_16bpp(hdcDest, nXDest, nYDest, nWidth, nHeight);
			break;

		case PATPAINT:
			return BitBlt_PATPAINT_16bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;
	}
	
	printf("BitBlt: unknown rop: 0x%08X\n", rop);
	return 1;
}

int PatBlt_16bpp(HDC hdc, int nXLeft, int nYLeft, int nWidth, int nHeight, int rop)
{
	if (ClipCoords(hdc, &nXLeft, &nYLeft, &nWidth, &nHeight, NULL, NULL) == 0)
		return 0;
	
	InvalidateRegion(hdc, nXLeft, nYLeft, nWidth, nHeight);

	switch (rop)
	{
		case PATCOPY:
			return BitBlt_PATCOPY_16bpp(hdc, nXLeft, nYLeft, nWidth, nHeight);
			break;

		case PATINVERT:
			return BitBlt_PATINVERT_16bpp(hdc, nXLeft, nYLeft, nWidth, nHeight);
			break;
			
		case DSTINVERT:
			return BitBlt_DSTINVERT_16bpp(hdc, nXLeft, nYLeft, nWidth, nHeight);
			break;

		case BLACKNESS:
			return BitBlt_BLACKNESS_16bpp(hdc, nXLeft, nYLeft, nWidth, nHeight);
			break;

		case WHITENESS:
			return BitBlt_WHITENESS_16bpp(hdc, nXLeft, nYLeft, nWidth, nHeight);
			break;
	}
	
	printf("PatBlt: unknown rop: 0x%08X", rop);
	return 1;
}

void SetPixel_BLACK_16bpp(uint16 *pixel, uint16 *pen)
{
	/* D = 0 */
	*pixel = 0;
}

void SetPixel_NOTMERGEPEN_16bpp(uint16 *pixel, uint16 *pen)
{
	/* D = ~(D | P) */
	*pixel = ~(*pixel | *pen);
}

void SetPixel_MASKNOTPEN_16bpp(uint16 *pixel, uint16 *pen)
{
	/* D = D & ~P */
	*pixel &= ~(*pen);
}

void SetPixel_NOTCOPYPEN_16bpp(uint16 *pixel, uint16 *pen)
{
	/* D = ~P */
	*pixel = ~(*pen);
}

void SetPixel_MASKPENNOT_16bpp(uint16 *pixel, uint16 *pen)
{
	/* D = P & ~D */
	*pixel = *pen & ~*pixel;
}

void SetPixel_NOT_16bpp(uint16 *pixel, uint16 *pen)
{
	/* D = ~D */
	*pixel = ~(*pixel);
}

void SetPixel_XORPEN_16bpp(uint16 *pixel, uint16 *pen)
{
	/* D = D ^ P */
	*pixel = *pixel ^ *pen;
}

void SetPixel_NOTMASKPEN_16bpp(uint16 *pixel, uint16 *pen)
{
	/* D = ~(D & P) */
	*pixel = ~(*pixel & *pen);
}

void SetPixel_MASKPEN_16bpp(uint16 *pixel, uint16 *pen)
{
	/* D = D & P */
	*pixel &= *pen;
}

void SetPixel_NOTXORPEN_16bpp(uint16 *pixel, uint16 *pen)
{
	/* D = ~(D ^ P) */
	*pixel = ~(*pixel ^ *pen);
}

void SetPixel_NOP_16bpp(uint16 *pixel, uint16 *pen)
{
	/* D = D */
}

void SetPixel_MERGENOTPEN_16bpp(uint16 *pixel, uint16 *pen)
{
	/* D = D | ~P */
	*pixel |= ~(*pen);
}

void SetPixel_COPYPEN_16bpp(uint16 *pixel, uint16 *pen)
{
	/* D = P */
	*pixel = *pen;
}

void SetPixel_MERGEPENNOT_16bpp(uint16 *pixel, uint16 *pen)
{
	/* D = P | ~D */
	*pixel = *pen | ~(*pixel);
}

void SetPixel_MERGEPEN_16bpp(uint16 *pixel, uint16 *pen)
{
	/* D = P | D */
	*pixel |= *pen;
}

void SetPixel_WHITE_16bpp(uint16 *pixel, uint16 *pen)
{
	/* D = 1 */
	*pixel = 0xFFFF;
}

pSetPixel16_ROP2 SetPixel16_ROP2_[16] =
{
	SetPixel_BLACK_16bpp,
	SetPixel_NOTMERGEPEN_16bpp,
	SetPixel_MASKNOTPEN_16bpp,
	SetPixel_NOTCOPYPEN_16bpp,
	SetPixel_MASKPENNOT_16bpp,
	SetPixel_NOT_16bpp,
	SetPixel_XORPEN_16bpp,
	SetPixel_NOTMASKPEN_16bpp,
	SetPixel_MASKPEN_16bpp,
	SetPixel_NOTXORPEN_16bpp,
	SetPixel_NOP_16bpp,
	SetPixel_MERGENOTPEN_16bpp,
	SetPixel_COPYPEN_16bpp,
	SetPixel_MERGEPENNOT_16bpp,
	SetPixel_MERGEPEN_16bpp,
	SetPixel_WHITE_16bpp
};

int LineTo_16bpp(HDC hdc, int nXEnd, int nYEnd)
{
	int x, y;
	int x1, y1;
	int x2, y2;
	int e, e2;
	int dx, dy;
	int sx, sy;
	HBITMAP bmp;
	int bx1, by1;
	int bx2, by2;

	int irop2;
	uint16 pen;
	uint16 *pixel;

	x1 = hdc->pen->posX;
	y1 = hdc->pen->posY;
	x2 = nXEnd;
	y2 = nYEnd;

	dx = (x1 > x2) ? x1 - x2 : x2 - x1;
	dy = (y1 > y2) ? y1 - y2 : y2 - y1;

	sx = (x1 < x2) ? 1 : -1;
	sy = (y1 < y2) ? 1 : -1;

	e = dx - dy;

	x = x1;
	y = y1;

	irop2 = GetROP2(hdc) - 1;
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

	pen = GetPenColor_16bpp(hdc->pen);

	while (1)
	{
		if (!(x == x2 && y == y2))
		{
			if ((x >= bx1 && x <= bx2) && (y >= by1 && y <= by2))
			{
				pixel = GetPointer_16bpp(bmp, x, y);
				SetPixel16_ROP2_[irop2](pixel, &pen);
			}
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

	return 1;
}
