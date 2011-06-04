/*
   FreeRDP: A Remote Desktop Protocol client.
   GDI Region Functions

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

#ifndef __GDI_REGION_H
#define __GDI_REGION_H

#include "gdi.h"

HRGN gdi_CreateRectRgn(int nLeftRect, int nTopRect, int nRightRect, int nBottomRect);
HRECT gdi_CreateRect(int xLeft, int yTop, int xRight, int yBottom);
void gdi_RectToRgn(HRECT rect, HRGN rgn);
void gdi_CRectToRgn(int left, int top, int right, int bottom, HRGN rgn);
void gdi_RectToCRgn(HRECT rect, int *x, int *y, int *w, int *h);
void gdi_CRectToCRgn(int left, int top, int right, int bottom, int *x, int *y, int *w, int *h);
void gdi_RgnToRect(HRGN rgn, HRECT rect);
void gdi_CRgnToRect(int x, int y, int w, int h, HRECT rect);
void gdi_RgnToCRect(HRGN rgn, int *left, int *top, int *right, int *bottom);
void gdi_CRgnToCRect(int x, int y, int w, int h, int *left, int *top, int *right, int *bottom);
int gdi_CopyOverlap(int x, int y, int width, int height, int srcx, int srcy);
int gdi_SetRect(HRECT rc, int xLeft, int yTop, int xRight, int yBottom);
int gdi_SetRgn(HRGN hRgn, int nXLeft, int nYLeft, int nWidth, int nHeight);
int gdi_SetRectRgn(HRGN hRgn, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect);
int gdi_EqualRgn(HRGN hSrcRgn1, HRGN hSrcRgn2);
int gdi_CopyRect(HRECT dst, HRECT src);
int gdi_PtInRect(HRECT rc, int x, int y);
int gdi_InvalidateRegion(HDC hdc, int x, int y, int w, int h);

#endif /* __GDI_REGION_H */
