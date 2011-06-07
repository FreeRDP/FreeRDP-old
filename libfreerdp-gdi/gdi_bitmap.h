/*
   FreeRDP: A Remote Desktop Protocol client.
   GDI Bitmap Functions

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

#ifndef __GDI_BITMAP_H
#define __GDI_BITMAP_H

#include "gdi.h"

GDI_COLOR gdi_GetPixel(HGDI_DC hdc, int nXPos, int nYPos);
GDI_COLOR gdi_SetPixel(HGDI_DC hdc, int X, int Y, GDI_COLOR crColor);
uint8 gdi_GetPixel_8bpp(HGDI_BITMAP hBmp, int X, int Y);
uint16 gdi_GetPixel_16bpp(HGDI_BITMAP hBmp, int X, int Y);
uint32 gdi_GetPixel_32bpp(HGDI_BITMAP hBmp, int X, int Y);
uint8* gdi_GetPointer_8bpp(HGDI_BITMAP hBmp, int X, int Y);
uint16* gdi_GetPointer_16bpp(HGDI_BITMAP hBmp, int X, int Y);
uint32* gdi_GetPointer_32bpp(HGDI_BITMAP hBmp, int X, int Y);
void gdi_SetPixel_8bpp(HGDI_BITMAP hBmp, int X, int Y, uint8 pixel);
void gdi_SetPixel_16bpp(HGDI_BITMAP hBmp, int X, int Y, uint16 pixel);
void gdi_SetPixel_32bpp(HGDI_BITMAP hBmp, int X, int Y, uint32 pixel);
HGDI_BITMAP gdi_CreateBitmap(int nWidth, int nHeight, int cBitsPerPixel, uint8* data);
HGDI_BITMAP gdi_CreateCompatibleBitmap(HGDI_DC hdc, int nWidth, int nHeight);
int gdi_BitBlt(HGDI_DC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HGDI_DC hdcSrc, int nXSrc, int nYSrc, int rop);

typedef int (*pBitBlt)(HGDI_DC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HGDI_DC hdcSrc, int nXSrc, int nYSrc, int rop);

#endif /* __GDI_BITMAP_H */
