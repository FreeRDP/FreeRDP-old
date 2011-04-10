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

HRGN CreateRectRgn(int nLeftRect, int nTopRect, int nRightRect, int nBottomRect);
HRECT CreateRect(int xLeft, int yTop, int xRight, int yBottom);
void RectToRgn(HRECT rect, HRGN rgn);
void CRectToRgn(int left, int top, int right, int bottom, HRGN rgn);
void RectToCRgn(HRECT rect, int *x, int *y, int *w, int *h);
void CRectToCRgn(int left, int top, int right, int bottom, int *x, int *y, int *w, int *h);
void RgnToRect(HRGN rgn, HRECT rect);
void CRgnToRect(int x, int y, int w, int h, HRECT rect);
void RgnToCRect(HRGN rgn, int *left, int *top, int *right, int *bottom);
void CRgnToCRect(int x, int y, int w, int h, int *left, int *top, int *right, int *bottom);
int CopyOverlap(int x, int y, int width, int height, int srcx, int srcy);
int SetRect(HRECT rc, int xLeft, int yTop, int xRight, int yBottom);
int SetRgn(HRGN hRgn, int nXLeft, int nYLeft, int nWidth, int nHeight);
int SetRectRgn(HRGN hRgn, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect);
int EqualRgn(HRGN hSrcRgn1, HRGN hSrcRgn2);
int CopyRect(HRECT dst, HRECT src);
int PtInRect(HRECT rc, int x, int y);
int InvalidateRegion(HDC hdc, int x, int y, int w, int h);

#endif /* __GDI_REGION_H */
