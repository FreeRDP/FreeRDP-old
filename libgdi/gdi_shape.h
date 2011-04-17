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

#ifndef __GDI_SHAPE_H
#define __GDI_SHAPE_H

#include "gdi.h"

int Ellipse(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect);
int FillRect(HDC hdc, HRECT rect, HBRUSH hbr);
int Polygon(HDC hdc, POINT *lpPoints, int nCount);
int PolyPolygon(HDC hdc, POINT *lpPoints, int *lpPolyCounts, int nCount);
int Rectangle(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect);

typedef int (*pFillRect)(HDC hdc, HRECT rect, HBRUSH hbr);

#endif /* __GDI_SHAPE_H */
