/*
   FreeRDP: A Remote Desktop Protocol client.
   32bpp Internal Buffer Routines

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
#include "gdi_32bpp.h"

int FillRect_32bpp(HDC hdc, HRECT rect, HBRUSH hbr)
{
	int x, y;
	uint8 *dstp;
	char r, g, b;
	int nXDest, nYDest;
	int nWidth, nHeight;

	RectToCRgn(rect, &nXDest, &nYDest, &nWidth, &nHeight);
	
	if (ClipCoords(hdc, &nXDest, &nYDest, &nWidth, &nHeight, NULL, NULL) == 0)
		return 0;

	GetRGB(r, g, b, hbr->color);
	
	for (y = 0; y < nHeight; y++)
	{
		dstp = gdi_get_bitmap_pointer(hdc, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				*dstp = b;
				dstp++;
					
				*dstp = g;
				dstp++;

				*dstp = r;
#ifdef USE_ALPHA
				dstp++;
				*dstp = 0xFF;
				dstp++;
#else
				dstp += 2;
#endif
			}
		}
	}

	InvalidateRegion(hdc, nXDest, nYDest, nWidth, nHeight);
	return 0;
}

static int BitBlt_BLACKNESS_32bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight)
{
	int y;
	uint8 *dstp;

#ifdef USE_ALPHA
	int x;
	for (y = 0; y < nHeight; y++)
	{
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				*dstp = 0;
				dstp++;

				*dstp = 0;
				dstp++;

				*dstp = 0;
				dstp++;

				*dstp = 0xFF;
				dstp++;
			}
		}
	}
#else
	for (y = 0; y < nHeight; y++)
	{
		dstp = gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
			memset(dstp, 0, nWidth * hdcDest->bytesPerPixel);
	}
#endif

	return 0;
}

static int BitBlt_WHITENESS_32bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight)
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

static int BitBlt_SRCCOPY_32bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
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

static int BitBlt_NOTSRCCOPY_32bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
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

				*dstp = ~(*srcp);
				srcp += 2;
				dstp += 2;
			}
		}
	}

	return 0;
}

static int BitBlt_DSTINVERT_32bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight)
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

				*dstp = ~(*dstp);
				dstp += 2;
			}
		}
	}

	return 0;
}

static int BitBlt_SRCERASE_32bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
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

				*dstp = *srcp & ~(*dstp);
				srcp += 2;
				dstp += 2;
			}
		}
	}

	return 0;
}

static int BitBlt_NOTSRCERASE_32bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
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

				*dstp = ~(*srcp) & ~(*dstp);
				srcp += 2;
				dstp += 2;
			}
		}
	}

	return 0;
}

static int BitBlt_SRCINVERT_32bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
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

				*dstp ^= *srcp;
				srcp += 2;
				dstp += 2;
			}
		}
	}

	return 0;
}

static int BitBlt_SRCAND_32bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
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

				*dstp &= *srcp;
				srcp += 2;
				dstp += 2;
			}
		}
	}

	return 0;
}

static int BitBlt_SRCPAINT_32bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
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

				*dstp |= *srcp;
				srcp += 2;
				dstp += 2;
			}
		}
	}

	return 0;
}

static int BitBlt_DSPDxax_32bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
{	
	int x, y;
	uint8 *srcp;
	uint32 *dstp;
	HBITMAP hSrcBmp;

	/* D = (S & P) | (~S & D)	*/
	/* DSPDxax, used to draw glyphs */
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
		dstp = (uint32*)gdi_get_bitmap_pointer(hdcDest, nXDest, nYDest + y);

		if (dstp != 0)
		{
			for (x = 0; x < nWidth; x++)
			{
				*dstp = (*srcp & hdcDest->textColor) | (~(*srcp) & *dstp);
				dstp++;
				srcp++;
			}
		}
	}

	return 0;
}

static int BitBlt_SPna_32bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
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

				*dstp = *srcp & ~(*patp);
				srcp += 2;
				dstp += 2;
			}
		}
	}
	
	return 0;
}

static int BitBlt_MERGECOPY_32bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
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

				*dstp = *srcp & *patp;
				dstp += 2;
				srcp += 2;
			}
		}
	}
	
	return 0;
}

static int BitBlt_MERGEPAINT_32bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
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

				*dstp = ~(*srcp) | *dstp;
				dstp += 2;
				srcp += 2;
			}
		}
	}
	
	return 0;
}

static int BitBlt_PATCOPY_32bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight)
{
	int x, y;
	uint8 *dstp;
	uint8 *patp;
		
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

				*dstp = *patp;
				dstp += 2;
			}
		}
	}
	
	return 0;
}

static int BitBlt_PATINVERT_32bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight)
{
	int x, y;
	uint8 *dstp;
	uint8 *patp;
		
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

				*dstp = *patp ^ *dstp;
				dstp += 2;
			}
		}
	}
	
	return 0;
}

static int BitBlt_PATPAINT_32bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc)
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

				*dstp = *dstp | (*patp | ~(*srcp));
				dstp += 2;
				srcp += 2;
			}
		}
	}
	
	return 0;
}

int BitBlt_32bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, int rop)
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
			return BitBlt_BLACKNESS_32bpp(hdcDest, nXDest, nYDest, nWidth, nHeight);
			break;

		case WHITENESS:
			return BitBlt_WHITENESS_32bpp(hdcDest, nXDest, nYDest, nWidth, nHeight);
			break;

		case SRCCOPY:
			return BitBlt_SRCCOPY_32bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case SPna:
			return BitBlt_SPna_32bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case DSPDxax:
			return BitBlt_DSPDxax_32bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;
			
		case NOTSRCCOPY:
			return BitBlt_NOTSRCCOPY_32bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case DSTINVERT:
			return BitBlt_DSTINVERT_32bpp(hdcDest, nXDest, nYDest, nWidth, nHeight);
			break;

		case SRCERASE:
			return BitBlt_SRCERASE_32bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case NOTSRCERASE:
			return BitBlt_NOTSRCERASE_32bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case SRCINVERT:
			return BitBlt_SRCINVERT_32bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case SRCAND:
			return BitBlt_SRCAND_32bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case SRCPAINT:
			return BitBlt_SRCPAINT_32bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case MERGECOPY:
			return BitBlt_MERGECOPY_32bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case MERGEPAINT:
			return BitBlt_MERGEPAINT_32bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;

		case PATCOPY:
			return BitBlt_PATCOPY_32bpp(hdcDest, nXDest, nYDest, nWidth, nHeight);
			break;

		case PATINVERT:
			return BitBlt_PATINVERT_32bpp(hdcDest, nXDest, nYDest, nWidth, nHeight);
			break;

		case PATPAINT:
			return BitBlt_PATPAINT_32bpp(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc);
			break;
	}
	
	printf("BitBlt: unknown rop: 0x%08X\n", rop);
	return 1;
}

int PatBlt_32bpp(HDC hdc, int nXLeft, int nYLeft, int nWidth, int nHeight, int rop)
{
	if (ClipCoords(hdc, &nXLeft, &nYLeft, &nWidth, &nHeight, NULL, NULL) == 0)
		return 0;
	
	InvalidateRegion(hdc, nXLeft, nYLeft, nWidth, nHeight);

	switch (rop)
	{
		case PATCOPY:
			return BitBlt_PATCOPY_32bpp(hdc, nXLeft, nYLeft, nWidth, nHeight);
			break;

		case PATINVERT:
			return BitBlt_PATINVERT_32bpp(hdc, nXLeft, nYLeft, nWidth, nHeight);
			break;
			
		case DSTINVERT:
			return BitBlt_DSTINVERT_32bpp(hdc, nXLeft, nYLeft, nWidth, nHeight);
			break;

		case BLACKNESS:
			return BitBlt_BLACKNESS_32bpp(hdc, nXLeft, nYLeft, nWidth, nHeight);
			break;

		case WHITENESS:
			return BitBlt_WHITENESS_32bpp(hdc, nXLeft, nYLeft, nWidth, nHeight);
			break;
	}
	
	printf("PatBlt: unknown rop: 0x%08X", rop);
	return 1;
}

