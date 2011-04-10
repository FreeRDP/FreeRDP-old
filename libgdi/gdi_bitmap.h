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

void BitmapInit();
COLORREF GetPixel(HDC hdc, int nXPos, int nYPos);
COLORREF SetPixel(HDC hdc, int X, int Y, COLORREF crColor);
void SetPixel_8bpp(HBITMAP hBmp, int X, int Y, uint8 pixel);
void SetPixel_16bpp(HBITMAP hBmp, int X, int Y, uint16 pixel);
void SetPixel_32bpp(HBITMAP hBmp, int X, int Y, uint32 pixel);
HBITMAP CreateBitmap(int nWidth, int nHeight, int cBitsPerPixel, uint8* data);
HBITMAP CreateCompatibleBitmap(HDC hdc, int nWidth, int nHeight);
int BitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, int rop);

#endif /* __GDI_BITMAP_H */
