/*
   FreeRDP: A Remote Desktop Protocol client.
   GDI Line Functions

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

#ifndef __GDI_LINE_H
#define __GDI_LINE_H

#include "gdi.h"

int LineTo(HDC hdc, int nXEnd, int nYEnd);
int PolylineTo(HDC hdc, POINT *lppt, int cCount);
int Polyline(HDC hdc, POINT *lppt, int cPoints);
int PolyPolyline(HDC hdc, POINT *lppt, int *lpdwPolyPoints, int cCount);
int MoveToEx(HDC hdc, int X, int Y, HPOINT lpPoint);

typedef int (*pLineTo)(HDC hdc, int nXEnd, int nYEnd);

#endif /* __GDI_LINE_H */
